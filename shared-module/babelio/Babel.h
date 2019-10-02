#ifndef MICROPY_INCLUDED_SHARED_MODULE_BABELIO_BABEL_H
#define MICROPY_INCLUDED_SHARED_MODULE_BABELIO_BABEL_H
 
#include "py/obj.h"
#include "common-hal/busio/SPI.h"
#include "common-hal/digitalio/DigitalInOut.h"
#include "shared-module/displayio/Bitmap.h"
#include "supervisor/shared/external_flash/devices.h"

typedef struct {
    int16_t x;
    int16_t y;
} _babelio_point_t;

typedef struct {
    int16_t min_x;
    int16_t min_y;
    int16_t max_x;
    int16_t max_y;
} _babelio_area_t;

typedef struct {
    mp_obj_base_t base;

    // interface to SPI chip. we get unicode stuff from this.
    digitalio_digitalinout_obj_t cs;
    busio_spi_obj_t *spi;
    uint32_t spi_flash_baudrate;

    // and we push pixels to this.
    displayio_bitmap_t *bitmap;

    // cached information about what's on the chip
    uint8_t nominal_width;
    uint8_t nominal_height;
    uint32_t last_codepoint;
    uint32_t location_of_lut;
    uint32_t location_of_glyphs;
    uint32_t location_of_extras;
    uint32_t start_of_uppercase_mapping;
    uint32_t start_of_lowercase_mapping;
    uint32_t start_of_titlecase_mapping;
    uint32_t start_of_mirrored_mapping;
    uint32_t end_of_uppercase_mapping;
    uint32_t end_of_lowercase_mapping;
    uint32_t end_of_titlecase_mapping;
    uint32_t end_of_mirrored_mapping;
    uint32_t info_for_replacement_character;
    uint16_t extended_info_for_replacement_character;

    // State of the typesetter
    _babelio_point_t cursor;
    int8_t  direction;
    uint16_t text_color;
    uint16_t text_size;
    bool bold;
    bool italic;
    _babelio_area_t display_area;
    _babelio_point_t last_glyph_location;
    bool has_last_glyph;
} babelio_babel_obj_t;
 

bool shared_module_babelio_babel_wait_for_flash_ready(babelio_babel_obj_t* self);
bool shared_module_babelio_babel_transfer(babelio_babel_obj_t* self, uint8_t* command, uint32_t command_length, void* data_in, void* data_out, uint32_t data_length);
bool shared_module_babelio_babel_transfer_command(babelio_babel_obj_t* self, uint8_t command, void* data_in, void* data_out, uint32_t data_length);
bool shared_module_babelio_babel_spi_flash_command(babelio_babel_obj_t* self, uint8_t command);
bool shared_module_babelio_babel_spi_flash_read_command(babelio_babel_obj_t* self, uint8_t command, void* data, uint32_t data_length);
bool shared_module_babelio_babel_spi_flash_write_command(babelio_babel_obj_t* self, uint8_t command, void* data, uint32_t data_length);
bool shared_module_babelio_babel_spi_flash_read_data(babelio_babel_obj_t* self, uint32_t address, void* data, uint32_t data_length);

#endif // MICROPY_INCLUDED_SHARED_MODULE_BABELIO_BABEL_H

