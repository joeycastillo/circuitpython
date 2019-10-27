#include "py/runtime.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/displayio/Bitmap.h"
#include "supervisor/shared/external_flash/common_commands.h"
#include "shared-module/babelio/Babel.h"

#define BABEL_HEADER_LOC_RESERVED 0
#define BABEL_HEADER_LOC_VERSION 2
#define BABEL_HEADER_LOC_WIDTH 4
#define BABEL_HEADER_LOC_HEIGHT 5
#define BABEL_HEADER_LOC_FLAGS 6
#define BABEL_HEADER_LOC_MAXGLYPH 8
#define BABEL_HEADER_LOC_START_OF_LUT 12
#define BABEL_HEADER_LOC_START_OF_GLYPHS 16
#define BABEL_HEADER_LOC_START_OF_EXTRAS 20

#define BABEL_HEADER_EXTRA_TYPE_UPPERCASE_MAPPINGS 1
#define BABEL_HEADER_EXTRA_TYPE_LOWERCASE_MAPPINGS 2
#define BABEL_HEADER_EXTRA_TYPE_TITLECASE_MAPPINGS 3
#define BABEL_HEADER_EXTRA_TYPE_MIRRORING_MAPPINGS 4

const external_flash_device _babel_device_descriptor = GD25Q16C;

// function signatures
bool shared_module_babelio_babel_wait_for_flash_ready(babelio_babel_obj_t* self);
bool shared_module_babelio_babel_transfer(babelio_babel_obj_t* self, uint8_t* command, uint32_t command_length, void* data_in, void* data_out, uint32_t data_length);
bool shared_module_babelio_babel_transfer_command(babelio_babel_obj_t* self, uint8_t command, void* data_in, void* data_out, uint32_t data_length);
bool shared_module_babelio_babel_spi_flash_command(babelio_babel_obj_t* self, uint8_t command);
bool shared_module_babelio_babel_spi_flash_read_command(babelio_babel_obj_t* self, uint8_t command, void* data, uint32_t data_length);
bool shared_module_babelio_babel_spi_flash_write_command(babelio_babel_obj_t* self, uint8_t command, void* data, uint32_t data_length);
bool shared_module_babelio_babel_spi_flash_read_data(babelio_babel_obj_t* self, uint32_t address, void* data, uint32_t data_length);
uint32_t shared_module_babelio_babel_fetch_glyph_basic_info(babelio_babel_obj_t* self, unichar codepoint);
uint16_t shared_module_babelio_babel_fetch_glyph_extended_info(babelio_babel_obj_t* self, unichar codepoint);


static void shared_module_babelio_address_to_bytes(uint32_t address, uint8_t* bytes) {
    bytes[0] = (address >> 16) & 0xff;
    bytes[1] = (address >> 8) & 0xff;
    bytes[2] = address & 0xff;
}

void shared_module_babelio_babel_flash_enable(babelio_babel_obj_t* self) {
//     return true;
//     while (!common_hal_busio_spi_try_lock(self->spi)) {}
    common_hal_digitalio_digitalinout_set_value(&self->cs, false);
}

// Disable the flash over SPI.
void shared_module_babelio_babel_flash_disable(babelio_babel_obj_t* self) {
    common_hal_digitalio_digitalinout_set_value(&self->cs, true);
//     common_hal_busio_spi_unlock(self->spi);
}

bool shared_module_babelio_babel_wait_for_flash_ready(babelio_babel_obj_t* self) {
    uint8_t read_status_response[1] = {0x00};
    bool ok = true;
    // Both the write enable and write in progress bits should be low.
    do {
        ok = shared_module_babelio_babel_spi_flash_read_command(self, CMD_READ_STATUS, read_status_response, 1);
    } while (ok && (read_status_response[0] & 0x3) != 0);
    return ok;
}

bool shared_module_babelio_babel_transfer(babelio_babel_obj_t* self, uint8_t* command, uint32_t command_length, void* data_in, void* data_out, uint32_t data_length) {
    shared_module_babelio_babel_flash_enable(self);
    bool status = common_hal_busio_spi_write(self->spi, command, command_length);
    if (status) {
        if (data_in != NULL && data_out != NULL) {
            status = common_hal_busio_spi_transfer(self->spi, data_out, data_in, data_length);
        } else if (data_out != NULL) {
            status = common_hal_busio_spi_read(self->spi, data_out, data_length, 0xff);
        } else if (data_in != NULL) {
            status = common_hal_busio_spi_write(self->spi, data_in, data_length);
        }
    }
    shared_module_babelio_babel_flash_disable(self);
    return status;
}

bool shared_module_babelio_babel_transfer_command(babelio_babel_obj_t* self, uint8_t command, void* data_in, void* data_out, uint32_t data_length) {
    return shared_module_babelio_babel_transfer(self, &command, 1, data_in, data_out, data_length);
}

bool shared_module_babelio_babel_spi_flash_command(babelio_babel_obj_t* self, uint8_t command) {
    return shared_module_babelio_babel_transfer_command(self, command, NULL, NULL, 0);
}

bool shared_module_babelio_babel_spi_flash_read_command(babelio_babel_obj_t* self, uint8_t command, void* data, uint32_t data_length) {
    return shared_module_babelio_babel_transfer_command(self, command, NULL, data, data_length);
}

bool shared_module_babelio_babel_spi_flash_write_command(babelio_babel_obj_t* self, uint8_t command, void* data, uint32_t data_length) {
    return shared_module_babelio_babel_transfer_command(self, command, data, NULL, data_length);
}

bool shared_module_babelio_babel_spi_flash_read_data(babelio_babel_obj_t* self, uint32_t address, void* data, uint32_t data_length) {
    uint8_t request[5] = {CMD_READ_DATA, 0x00, 0x00, 0x00};
    uint8_t command_length = 4;
    if (_babel_device_descriptor.supports_fast_read) {
        request[0] = CMD_FAST_READ_DATA;
        command_length = 5;
    }
    // Write the SPI flash write address into the bytes following the command byte.
    shared_module_babelio_address_to_bytes(address, request + 1);
    shared_module_babelio_babel_flash_enable(self);
    common_hal_busio_spi_configure(self->spi, self->spi_flash_baudrate, 0, 0, 8);
    bool status = common_hal_busio_spi_write(self->spi, request, command_length);
    if (status) {
        status = common_hal_busio_spi_read(self->spi, data, data_length, 0xff);
    }
    shared_module_babelio_babel_flash_disable(self);
    return status;
}

void shared_module_babelio_babel_construct(babelio_babel_obj_t* self, mp_obj_t spi_in, mp_obj_t cs_in, mp_obj_t bitmap_in) {
    self->spi = MP_OBJ_TO_PTR(spi_in);
    common_hal_digitalio_digitalinout_construct(&self->cs, cs_in);
    common_hal_digitalio_digitalinout_switch_to_output(&self->cs, 1, DRIVE_MODE_PUSH_PULL);
    common_hal_digitalio_digitalinout_never_reset(&self->cs);
    common_hal_mcu_delay_us(_babel_device_descriptor.start_up_time_us);

    // The response will be 0xff if the flash needs more time to start up.
    uint8_t jedec_id_response[3] = {0xff, 0xff, 0xff};
    while (jedec_id_response[0] == 0xff) {
        shared_module_babelio_babel_spi_flash_read_command(self, CMD_READ_JEDEC_ID, jedec_id_response, 3);
        return;
    }    
    if (jedec_id_response[0] != _babel_device_descriptor.manufacturer_id) mp_raise_ValueError(translate("Invalid Flash manufacturer ID"));
    if (jedec_id_response[1] != _babel_device_descriptor.memory_type) mp_raise_ValueError(translate("Invalid Flash memory type"));
    if (jedec_id_response[2] != _babel_device_descriptor.capacity) mp_raise_ValueError(translate("Invalid Flash capacity"));

    uint8_t read_status_response[1] = {0x00};
    // The write in progress bit should be low.
    do {
        shared_module_babelio_babel_spi_flash_read_command(self, CMD_READ_STATUS, read_status_response, 1);
    } while ((read_status_response[0] & 0x1) != 0);
    // The suspended write/erase bit should be low.
    do {
        shared_module_babelio_babel_spi_flash_read_command(self, CMD_READ_STATUS2, read_status_response, 1);
    } while ((read_status_response[0] & 0x80) != 0);

    shared_module_babelio_babel_spi_flash_command(self, CMD_ENABLE_RESET);
    shared_module_babelio_babel_spi_flash_command(self, CMD_RESET);

    // Wait 30us for the reset
    common_hal_mcu_delay_us(30);

    self->spi_flash_baudrate = _babel_device_descriptor.max_clock_speed_mhz * 1000000;
    if (self->spi_flash_baudrate > SPI_FLASH_MAX_BAUDRATE) {
        self->spi_flash_baudrate = SPI_FLASH_MAX_BAUDRATE;
    }

    // Turn off writes
    shared_module_babelio_babel_spi_flash_command(self, CMD_DISABLE_WRITE);
    shared_module_babelio_babel_wait_for_flash_ready(self);

    // if they gave us a bitmap for drawing pixels to, stash that too
    if (bitmap_in) self->bitmap = MP_OBJ_TO_PTR(bitmap_in);

    // read data from flash chip and initialize properties
    shared_module_babelio_babel_spi_flash_read_data(self, BABEL_HEADER_LOC_WIDTH, &self->nominal_width, sizeof(self->nominal_width));
    shared_module_babelio_babel_spi_flash_read_data(self, BABEL_HEADER_LOC_HEIGHT, &self->nominal_height, sizeof(self->nominal_height));
    shared_module_babelio_babel_spi_flash_read_data(self, BABEL_HEADER_LOC_MAXGLYPH, &self->last_codepoint, sizeof(self->last_codepoint));
    shared_module_babelio_babel_spi_flash_read_data(self, BABEL_HEADER_LOC_START_OF_LUT, &self->location_of_lut, sizeof(self->location_of_lut));
    shared_module_babelio_babel_spi_flash_read_data(self, BABEL_HEADER_LOC_START_OF_GLYPHS, &self->location_of_glyphs, sizeof(self->location_of_glyphs));
    shared_module_babelio_babel_spi_flash_read_data(self, BABEL_HEADER_LOC_START_OF_EXTRAS, &self->location_of_extras, sizeof(self->location_of_extras));
    uint32_t extra_loc = 0;
    uint32_t extra_len = 0;
    uint32_t currentPos = self->location_of_extras;
    do {
        shared_module_babelio_babel_spi_flash_read_data(self, currentPos, &extra_loc, sizeof(extra_loc));
        currentPos += sizeof(extra_loc);
        shared_module_babelio_babel_spi_flash_read_data(self, currentPos, &extra_len, sizeof(extra_loc));
        currentPos += sizeof(extra_len);
        switch (extra_loc & 0xFF) {
        case BABEL_HEADER_EXTRA_TYPE_UPPERCASE_MAPPINGS:
            self->start_of_uppercase_mapping = extra_loc >> 8;
            self->end_of_uppercase_mapping = (extra_loc >> 8) + (extra_len >> 8);
            break;
        case BABEL_HEADER_EXTRA_TYPE_LOWERCASE_MAPPINGS:
            self->start_of_lowercase_mapping = extra_loc >> 8;
            self->end_of_lowercase_mapping = (extra_loc >> 8) + (extra_len >> 8);
            break;
        case BABEL_HEADER_EXTRA_TYPE_TITLECASE_MAPPINGS:
            self->start_of_titlecase_mapping = extra_loc >> 8;
            self->end_of_titlecase_mapping = (extra_loc >> 8) + (extra_len >> 8);
            break;
        case BABEL_HEADER_EXTRA_TYPE_MIRRORING_MAPPINGS:
            self->start_of_mirrored_mapping = extra_loc >> 8;
            self->end_of_mirrored_mapping = (extra_loc >> 8) + (extra_len >> 8);
            break;
        }
    } while(extra_loc && (currentPos < 256));

    self->info_for_replacement_character = shared_module_babelio_babel_fetch_glyph_basic_info(self, 0xFFFD);
    self->extended_info_for_replacement_character = shared_module_babelio_babel_fetch_glyph_extended_info(self, 0xFFFD);

    // set up typesetter state
    self->cursor.x = 0;
    self->cursor.y = 0;
    self->direction = 1;
    self->text_color = 0;
    self->text_size = 1;
    self->bold = false;
    self->italic = false;
    self->display_area.min_x = 0;
    self->display_area.max_y = 0;
    self->display_area.min_x = 0;
    self->display_area.max_y = 0;
    self->last_glyph_location.x = 0;
    self->last_glyph_location.y = 0;
    self->has_last_glyph = false;
}

bool shared_module_babelio_babel_deinited(babelio_babel_obj_t* self) {
    return common_hal_digitalio_digitalinout_deinited(&self->cs);
}
 
void shared_module_babelio_babel_deinit(babelio_babel_obj_t* self) {
    common_hal_digitalio_digitalinout_deinit(&self->cs);
}

mp_int_t shared_module_babelio_babel_get_max_codepoint(babelio_babel_obj_t* self) {
    return self->last_codepoint;
}

uint32_t shared_module_babelio_babel_fetch_glyph_basic_info(babelio_babel_obj_t* self, unichar codepoint) {
    uint32_t retVal;
    uint32_t loc = self->location_of_lut + codepoint * 6;

    shared_module_babelio_babel_spi_flash_read_data(self, loc, &retVal, 4);

    return retVal;
}

uint16_t shared_module_babelio_babel_fetch_glyph_extended_info(babelio_babel_obj_t* self, unichar codepoint) {
    uint16_t retVal;
    uint32_t loc = 4 + self->location_of_lut + codepoint * 6;

    shared_module_babelio_babel_spi_flash_read_data(self, loc, &retVal, 2);

    return retVal;
}

bool shared_module_babelio_babel_fetch_glyph_data(babelio_babel_obj_t* self, unichar codepoint, _babelio_glyph_t *glyph) {
    bool retVal = true;
    uint32_t loc = self->location_of_lut + codepoint * 6;

    // don't bother looking up out-of-range codepoints
    if (codepoint > self->last_codepoint) glyph->info = 0;
    else shared_module_babelio_babel_spi_flash_read_data(self, loc, glyph, 6);

    if (!glyph->info) {
        glyph->info = self->info_for_replacement_character;
        glyph->extendedInfo = self->extended_info_for_replacement_character;
        retVal = false;
    }
    
    loc = BABEL_INFO_GET_GLYPH_LOCATION(glyph->info);

    if (BABEL_INFO_GET_GLYPH_WIDTH(glyph->info) == 16) {
        shared_module_babelio_babel_spi_flash_read_data(self, loc, &glyph->glyphData, 32);
        return retVal;
    } else {
        shared_module_babelio_babel_spi_flash_read_data(self, loc, &glyph->glyphData, 16);
        return retVal;
    }
}

int16_t shared_module_babelio_babel_search_mapping(babelio_babel_obj_t* self, uint32_t start_of_mapping, uint32_t first, uint32_t last, unichar key) {
    uint32_t retVal;
    if (start_of_mapping == 0 || first > last) {
        retVal = -1;
    } else {
        uint32_t mid = (first + last)/2;
        BABEL_MAPPING mapping;
        shared_module_babelio_babel_spi_flash_read_data(self, start_of_mapping + mid * sizeof(BABEL_MAPPING), &mapping, sizeof(BABEL_MAPPING));
        unichar current_key = BABEL_MAPPING_GET_KEY(mapping);
        if (current_key == key) {
            retVal = mid;
        } else {
            if (key < current_key) {
                retVal = shared_module_babelio_babel_search_mapping(self, start_of_mapping, first, mid - 1, key);
            } else {
                retVal = shared_module_babelio_babel_search_mapping(self, start_of_mapping, mid + 1, last, key);
            }
        }
    }

    return retVal;
}

unichar shared_module_babelio_babel_uppercase_mapping_for_codepoint(babelio_babel_obj_t* self, unichar codepoint) {
    uint32_t lastIndex = (self->end_of_uppercase_mapping - self->start_of_uppercase_mapping) / sizeof(BABEL_MAPPING);
    int16_t index_of_result = shared_module_babelio_babel_search_mapping(self, self->start_of_uppercase_mapping, 0, lastIndex, codepoint);

    if (index_of_result == -1) return codepoint;
    BABEL_MAPPING mapping;
    shared_module_babelio_babel_spi_flash_read_data(self, self->start_of_uppercase_mapping + index_of_result * sizeof(BABEL_MAPPING), &mapping, sizeof(BABEL_MAPPING));

    return BABEL_MAPPING_GET_VALUE(mapping);
}

unichar shared_module_babelio_babel_lowercase_mapping_for_codepoint(babelio_babel_obj_t* self, unichar codepoint) {
    uint32_t lastIndex = (self->end_of_lowercase_mapping - self->start_of_lowercase_mapping) / sizeof(BABEL_MAPPING);
    int16_t index_of_result = shared_module_babelio_babel_search_mapping(self, self->start_of_lowercase_mapping, 0, lastIndex, codepoint);

    if (index_of_result == -1) return codepoint;
    BABEL_MAPPING mapping;
    shared_module_babelio_babel_spi_flash_read_data(self, self->start_of_lowercase_mapping + index_of_result * sizeof(BABEL_MAPPING), &mapping, sizeof(BABEL_MAPPING));

    return BABEL_MAPPING_GET_VALUE(mapping);
}

unichar shared_module_babelio_babel_titlecase_mapping_for_codepoint(babelio_babel_obj_t* self, unichar codepoint) {
    uint32_t lastIndex = (self->end_of_titlecase_mapping - self->start_of_titlecase_mapping) / sizeof(BABEL_MAPPING);
    int16_t index_of_result = shared_module_babelio_babel_search_mapping(self, self->start_of_titlecase_mapping, 0, lastIndex, codepoint);

    if (index_of_result == -1) return codepoint;
    BABEL_MAPPING mapping;
    shared_module_babelio_babel_spi_flash_read_data(self, self->start_of_titlecase_mapping + index_of_result * sizeof(BABEL_MAPPING), &mapping, sizeof(BABEL_MAPPING));

    return BABEL_MAPPING_GET_VALUE(mapping);
}

void shared_module_babelio_babel_to_uppercase(babelio_babel_obj_t* self, unichar *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint16_t extendedInfo = shared_module_babelio_babel_fetch_glyph_extended_info(self, buf[i]);
        if (BABEL_EXTENDED_GET_HAS_UPPERCASE_MAPPING(extendedInfo)) {
            unichar uppercaseCodepoint = shared_module_babelio_babel_uppercase_mapping_for_codepoint(self, buf[i]);
            buf[i] = uppercaseCodepoint;
        }
    }
}

void shared_module_babelio_babel_to_lowercase(babelio_babel_obj_t* self, unichar *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint16_t extendedInfo = shared_module_babelio_babel_fetch_glyph_extended_info(self, buf[i]);
        if (BABEL_EXTENDED_GET_HAS_LOWERCASE_MAPPING(extendedInfo)) {
            unichar lowercaseCodepoint = shared_module_babelio_babel_lowercase_mapping_for_codepoint(self, buf[i]);
            buf[i] = lowercaseCodepoint;
        }
    }
}

void shared_module_babelio_babel_draw_fillrect(babelio_babel_obj_t* self, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t i=x; i<x+w; i++) {
        for (int16_t j=y; j<y+h; j++) {
            common_hal_displayio_bitmap_set_pixel(self->bitmap, i, j, color);
        }
    }
}