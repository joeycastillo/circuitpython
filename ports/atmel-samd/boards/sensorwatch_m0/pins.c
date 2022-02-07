#include "shared-bindings/board/__init__.h"

STATIC const mp_rom_map_elem_t board_global_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_A0), MP_ROM_PTR(&pin_PB04) },

    { MP_ROM_QSTR(MP_QSTR_A1), MP_ROM_PTR(&pin_PB01) },
    { MP_ROM_QSTR(MP_QSTR_SCK), MP_ROM_PTR(&pin_PB01) },

    { MP_ROM_QSTR(MP_QSTR_A2), MP_ROM_PTR(&pin_PB02) },
    { MP_ROM_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(&pin_PB02) },

    { MP_ROM_QSTR(MP_QSTR_A3), MP_ROM_PTR(&pin_PB03) },
    { MP_ROM_QSTR(MP_QSTR_CS), MP_ROM_PTR(&pin_PB03) },

    { MP_ROM_QSTR(MP_QSTR_A4), MP_ROM_PTR(&pin_PB00) },
    { MP_ROM_QSTR(MP_QSTR_MISO), MP_ROM_PTR(&pin_PB00) },

    { MP_ROM_QSTR(MP_QSTR_SDA), MP_ROM_PTR(&pin_PA30) },
    { MP_ROM_QSTR(MP_QSTR_SCL), MP_ROM_PTR(&pin_PA31) },

    { MP_ROM_QSTR(MP_QSTR_LED_RED), MP_ROM_PTR(&pin_PA20) },
    { MP_ROM_QSTR(MP_QSTR_LED_GREEN), MP_ROM_PTR(&pin_PA21) },

    { MP_ROM_QSTR(MP_QSTR_BUZZER), MP_ROM_PTR(&pin_PA27) },

    { MP_ROM_QSTR(MP_QSTR_BTN_LIGHT), MP_ROM_PTR(&pin_PA22) },
    { MP_ROM_QSTR(MP_QSTR_BTN_MODE), MP_ROM_PTR(&pin_PA23) },
    { MP_ROM_QSTR(MP_QSTR_BTN_ALARM), MP_ROM_PTR(&pin_PA02) },

    { MP_ROM_QSTR(MP_QSTR_BTN_VBUS_DET), MP_ROM_PTR(&pin_PB05) },

    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&board_spi_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&board_uart_obj) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_global_dict_table);
