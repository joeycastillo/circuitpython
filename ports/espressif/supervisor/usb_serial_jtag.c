/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Patrick Van Oosterwijck
 * Copyright (c) 2022 Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/ringbuf.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "usb_serial_jtag.h"

#include "hal/usb_serial_jtag_ll.h"
#include "esp_intr_alloc.h"
#include "soc/periph_defs.h"

#include "supervisor/esp_port.h"

#define USB_SERIAL_JTAG_BUF_SIZE (64)

STATIC ringbuf_t ringbuf;
STATIC uint8_t buf[128];
STATIC bool connected;

static void usb_serial_jtag_isr_handler(void *arg) {
    uint32_t flags = usb_serial_jtag_ll_get_intsts_mask();

    if (flags & USB_SERIAL_JTAG_INTR_SOF) {
        usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SOF);
    }

    if (flags & USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT) {
        usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
        size_t req_len = ringbuf_num_empty(&ringbuf);
        if (req_len > USB_SERIAL_JTAG_BUF_SIZE) {
            req_len = USB_SERIAL_JTAG_BUF_SIZE;
        }
        uint8_t rx_buf[USB_SERIAL_JTAG_BUF_SIZE];
        size_t len = usb_serial_jtag_ll_read_rxfifo(rx_buf, req_len);
        for (size_t i = 0; i < len; ++i) {
            if (rx_buf[i] == mp_interrupt_char) {
                mp_sched_keyboard_interrupt();
            } else {
                ringbuf_put(&ringbuf, rx_buf[i]);
            }
        }
        vTaskNotifyGiveFromISR(circuitpython_task, NULL);
    }
}

void usb_serial_jtag_init(void) {
    ringbuf_init(&ringbuf, buf, sizeof(buf));
    usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SOF | USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
    usb_serial_jtag_ll_ena_intr_mask(USB_SERIAL_JTAG_INTR_SOF | USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
    ESP_ERROR_CHECK(esp_intr_alloc(ETS_USB_SERIAL_JTAG_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1,
        usb_serial_jtag_isr_handler, NULL, NULL));
}

bool usb_serial_jtag_connected(void) {
    // Make connected sticky. Otherwise we'll be disconnected every time the SOF
    // index is 0. (It's only ~15 bits so it wraps around frequently.)
    if (connected) {
        return true;
    }
    connected = USB_SERIAL_JTAG.fram_num.sof_frame_index > 0;
    return connected;
}

char usb_serial_jtag_read_char(void) {
    if (ringbuf_num_filled(&ringbuf) == 0) {
        return -1;
    }
    return ringbuf_get(&ringbuf);
}

bool usb_serial_jtag_bytes_available(void) {
    return ringbuf_num_filled(&ringbuf);
}

void usb_serial_jtag_write(const char *text, uint32_t length) {
    if (USB_SERIAL_JTAG.fram_num.sof_frame_index > 0) {
        size_t total_written = 0;
        uint32_t start_time = supervisor_ticks_ms32();
        // Time out after 5 milliseconds in case usb isn't actually reading CDC.
        while (total_written < length && start_time - supervisor_ticks_ms32() < 5) {
            total_written += usb_serial_jtag_ll_write_txfifo((const uint8_t *)(text + total_written), length - total_written);
            RUN_BACKGROUND_TASKS;
        }
        usb_serial_jtag_ll_txfifo_flush();
    }
}
