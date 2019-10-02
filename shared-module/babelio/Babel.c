#include "py/runtime.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/microcontroller/__init__.h"
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

static void shared_module_babelio_address_to_bytes(uint32_t address, uint8_t* bytes) {
    bytes[0] = (address >> 16) & 0xff;
    bytes[1] = (address >> 8) & 0xff;
    bytes[2] = address & 0xff;
}

void shared_module_babelio_babel_flash_enable(babelio_babel_obj_t* self) {
    while (!common_hal_busio_spi_try_lock(self->spi)) {}
    common_hal_digitalio_digitalinout_set_value(&self->cs, false);
}

// Disable the flash over SPI.
void shared_module_babelio_babel_flash_disable(babelio_babel_obj_t* self) {
    common_hal_digitalio_digitalinout_set_value(&self->cs, true);
    common_hal_busio_spi_unlock(self->spi);
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

    // TODO: read data from flash chip and initialize properties
    self->nominal_width = 0;
    self->nominal_height = 0;
    shared_module_babelio_babel_spi_flash_read_data(self, BABEL_HEADER_LOC_MAXGLYPH, &self->last_codepoint, sizeof(self->last_codepoint));
    self->location_of_lut = 0;
    self->location_of_glyphs = 0;
    self->location_of_extras = 0;
    self->start_of_uppercase_mapping = 0;
    self->start_of_lowercase_mapping = 0;
    self->start_of_titlecase_mapping = 0;
    self->start_of_mirrored_mapping = 0;
    self->end_of_uppercase_mapping = 0;
    self->end_of_lowercase_mapping = 0;
    self->end_of_titlecase_mapping = 0;
    self->end_of_mirrored_mapping = 0;
    self->info_for_replacement_character = 0;
    self->extended_info_for_replacement_character = 0;

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

