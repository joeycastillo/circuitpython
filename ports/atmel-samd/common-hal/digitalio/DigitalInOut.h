/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
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

#ifndef MICROPY_INCLUDED_ATMEL_SAMD_COMMON_HAL_DIGITALIO_DIGITALINOUT_H
#define MICROPY_INCLUDED_ATMEL_SAMD_COMMON_HAL_DIGITALIO_DIGITALINOUT_H

#include "common-hal/microcontroller/Pin.h"
#include "py/obj.h"

typedef struct {
    mp_obj_base_t base;
    const mcu_pin_obj_t *pin;
    bool output;
    bool open_drain;
} digitalio_digitalinout_obj_t;

#ifdef SAML22
// these functions are missing from hri_port_l22.h; putting them here until i find a better place.
// joey 4/19/21
#include "hri_port_l22.h"

#define PORT_DIR_DIR_Pos            0            /**< \brief (PORT_DIR) Port Data Direction */
#define PORT_DIR_DIR_Msk            (_U_(0xFFFFFFFF) << PORT_DIR_DIR_Pos)
#define PORT_DIR_DIR(value)         (PORT_DIR_DIR_Msk & ((value) << PORT_DIR_DIR_Pos))
#define PORT_DIR_MASK               _U_(0xFFFFFFFF) /**< \brief (PORT_DIR) MASK Register */

#define PORT_OUT_OUT_Pos            0            /**< \brief (PORT_OUT) PORT Data Output Value */
#define PORT_OUT_OUT_Msk            (_U_(0xFFFFFFFF) << PORT_OUT_OUT_Pos)
#define PORT_OUT_OUT(value)         (PORT_OUT_OUT_Msk & ((value) << PORT_OUT_OUT_Pos))
#define PORT_OUT_MASK               _U_(0xFFFFFFFF) /**< \brief (PORT_OUT) MASK Register */

static inline void hri_port_set_DIR_DIR_bf(const void *const hw, uint8_t submodule_index, hri_port_dir_reg_t mask)
{
	((Port *)hw)->Group[submodule_index].DIRSET.reg = PORT_DIR_DIR(mask);
}

static inline void hri_port_clear_DIR_DIR_bf(const void *const hw, uint8_t submodule_index, hri_port_dir_reg_t mask)
{
	((Port *)hw)->Group[submodule_index].DIRCLR.reg = PORT_DIR_DIR(mask);
}

static inline void hri_port_set_OUT_OUT_bf(const void *const hw, uint8_t submodule_index, hri_port_out_reg_t mask)
{
	((Port *)hw)->Group[submodule_index].OUTSET.reg = PORT_OUT_OUT(mask);
}

static inline void hri_port_clear_OUT_OUT_bf(const void *const hw, uint8_t submodule_index, hri_port_out_reg_t mask)
{
	((Port *)hw)->Group[submodule_index].OUTCLR.reg = PORT_OUT_OUT(mask);
}
#endif

#endif // MICROPY_INCLUDED_ATMEL_SAMD_COMMON_HAL_DIGITALIO_DIGITALINOUT_H
