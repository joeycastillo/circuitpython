#include "shared-bindings/board/__init__.h"

#include "boards/board.h"
#include "shared-module/displayio/__init__.h"

STATIC const mp_rom_map_elem_t board_global_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_AUDIO),  MP_ROM_PTR(&pin_PA02) }, // A0 = audio mono channel
    { MP_OBJ_NEW_QSTR(MP_QSTR_MICRAW),  MP_ROM_PTR(&pin_PA05) }, // A1 = mic raw
    { MP_OBJ_NEW_QSTR(MP_QSTR_MICAMP),  MP_ROM_PTR(&pin_PA04) }, // A2 = mic amp
    { MP_OBJ_NEW_QSTR(MP_QSTR_ACCESSORY),  MP_ROM_PTR(&pin_PB02) }, // A3 = accessory
    { MP_OBJ_NEW_QSTR(MP_QSTR_VBAT),  MP_ROM_PTR(&pin_PA06) }, // A4 = vbat
    { MP_OBJ_NEW_QSTR(MP_QSTR_VBUS),  MP_ROM_PTR(&pin_PA07) }, // A5 = vbus

    // UART
    { MP_OBJ_NEW_QSTR(MP_QSTR_RX),  MP_ROM_PTR(&pin_PA01) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TX),  MP_ROM_PTR(&pin_PA00) },

    // Mic Shutdown
    { MP_OBJ_NEW_QSTR(MP_QSTR_SHDN),  MP_ROM_PTR(&pin_PA14) },

    // USB ID pin
    { MP_OBJ_NEW_QSTR(MP_QSTR_USBID),  MP_ROM_PTR(&pin_PA31) },

    // SD Chip Select
    { MP_OBJ_NEW_QSTR(MP_QSTR_SDCS),  MP_ROM_PTR(&pin_PA15) },

    // E-Paper control pins
    { MP_OBJ_NEW_QSTR(MP_QSTR_ECS),  MP_ROM_PTR(&pin_PA23) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EDC),  MP_ROM_PTR(&pin_PA22) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ERST),  MP_ROM_PTR(&pin_PA21) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EBSY),  MP_ROM_PTR(&pin_PA12) },

    // Blinkenlight
    { MP_OBJ_NEW_QSTR(MP_QSTR_LED),  MP_ROM_PTR(&pin_PA27) },

    // Touch slider
    { MP_OBJ_NEW_QSTR(MP_QSTR_SL1),  MP_ROM_PTR(&pin_PA20) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SL2),  MP_ROM_PTR(&pin_PA19) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SL3),  MP_ROM_PTR(&pin_PA18) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SL4),  MP_ROM_PTR(&pin_PA17) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SL5),  MP_ROM_PTR(&pin_PA16) },

    // I2C
    { MP_OBJ_NEW_QSTR(MP_QSTR_SDA),  MP_ROM_PTR(&pin_PB08) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCL),  MP_ROM_PTR(&pin_PB09) },

    // SPI
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCK),  MP_ROM_PTR(&pin_PB03) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MISO),  MP_ROM_PTR(&pin_PB22) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MOSI),  MP_ROM_PTR(&pin_PB23) },

    // Neopixel
    { MP_OBJ_NEW_QSTR(MP_QSTR_NEOPIXEL),  MP_ROM_PTR(&pin_PA30) },

    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&board_spi_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&board_uart_obj) },

    { MP_ROM_QSTR(MP_QSTR_DISPLAY), MP_ROM_PTR(&displays[0].epaper_display)}
};
MP_DEFINE_CONST_DICT(board_module_globals, board_global_dict_table);
