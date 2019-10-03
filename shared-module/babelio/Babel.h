#ifndef MICROPY_INCLUDED_SHARED_MODULE_BABELIO_BABEL_H
#define MICROPY_INCLUDED_SHARED_MODULE_BABELIO_BABEL_H
 
#include "py/obj.h"
#include "common-hal/busio/SPI.h"
#include "common-hal/digitalio/DigitalInOut.h"
#include "shared-module/displayio/Bitmap.h"
#include "supervisor/shared/external_flash/devices.h"
#include "shared-module/babelio/BabelMacros.h"

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
    uint32_t info;
    uint16_t extendedInfo;
    uint8_t glyphData[32];
} _babelio_glyph_t;

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

#endif // MICROPY_INCLUDED_SHARED_MODULE_BABELIO_BABEL_H

