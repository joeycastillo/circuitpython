// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2023 Bill Sideris, independently providing these changes.
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"

static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    // Left header, module facing up.
    { MP_ROM_QSTR(MP_QSTR_BTN_LOCK),    MP_ROM_PTR(&pin_GPIO0)  },
    { MP_ROM_QSTR(MP_QSTR_EPD_BUSY),    MP_ROM_PTR(&pin_GPIO1)  },
    { MP_ROM_QSTR(MP_QSTR_SDA),         MP_ROM_PTR(&pin_GPIO2) },
    { MP_ROM_QSTR(MP_QSTR_SCL),         MP_ROM_PTR(&pin_GPIO3) },
    { MP_ROM_QSTR(MP_QSTR_STATMON),     MP_ROM_PTR(&pin_GPIO4)  },
    { MP_ROM_QSTR(MP_QSTR_BATMON),      MP_ROM_PTR(&pin_GPIO5)  },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_INT),   MP_ROM_PTR(&pin_GPIO6)  },
    { MP_ROM_QSTR(MP_QSTR_BUSMON),      MP_ROM_PTR(&pin_GPIO7)  },
    { MP_ROM_QSTR(MP_QSTR_EPD_CS),      MP_ROM_PTR(&pin_GPIO9)  },
    { MP_ROM_QSTR(MP_QSTR_EPD_DC),      MP_ROM_PTR(&pin_GPIO9)  },
    { MP_ROM_QSTR(MP_QSTR_SD_CS),       MP_ROM_PTR(&pin_GPIO10) },
    { MP_ROM_QSTR(MP_QSTR_MOSI),        MP_ROM_PTR(&pin_GPIO11) },
    { MP_ROM_QSTR(MP_QSTR_SCK),         MP_ROM_PTR(&pin_GPIO12) },
    { MP_ROM_QSTR(MP_QSTR_MISO),        MP_ROM_PTR(&pin_GPIO13) },
    { MP_ROM_QSTR(MP_QSTR_SD_CD),       MP_ROM_PTR(&pin_GPIO14) },
    { MP_ROM_QSTR(MP_QSTR_EPD_MOSI),    MP_ROM_PTR(&pin_GPIO17) },
    { MP_ROM_QSTR(MP_QSTR_EPD_SCK),     MP_ROM_PTR(&pin_GPIO18) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_RESET), MP_ROM_PTR(&pin_GPIO21) },
    { MP_ROM_QSTR(MP_QSTR_EN2),         MP_ROM_PTR(&pin_GPIO38) },
    { MP_ROM_QSTR(MP_QSTR_EPD_RESET),   MP_ROM_PTR(&pin_GPIO46)  },
    { MP_ROM_QSTR(MP_QSTR_PWM_WARM),    MP_ROM_PTR(&pin_GPIO47)  },
    { MP_ROM_QSTR(MP_QSTR_PWM_COOL),    MP_ROM_PTR(&pin_GPIO48) },

    { MP_ROM_QSTR(MP_QSTR_TX),          MP_ROM_PTR(&pin_GPIO43) },
    { MP_ROM_QSTR(MP_QSTR_RX),          MP_ROM_PTR(&pin_GPIO44) },

    { MP_ROM_QSTR(MP_QSTR_I2C),         MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI),         MP_ROM_PTR(&board_spi_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART),        MP_ROM_PTR(&board_uart_obj) },
};

MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
