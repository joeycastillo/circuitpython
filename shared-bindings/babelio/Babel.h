#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_BABELIO_BABEL_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_BABELIO_BABEL_H
 
#include "shared-module/babelio/Babel.h"
 
extern const mp_obj_type_t babelio_babel_type;
 
extern void shared_module_babelio_babel_construct(babelio_babel_obj_t* self, mp_obj_t spi_in, mp_obj_t cs_in, mp_obj_t bitmap_in);
extern void shared_module_babelio_babel_deinit(babelio_babel_obj_t* self);
extern bool shared_module_babelio_babel_deinited(babelio_babel_obj_t* self);
extern mp_int_t shared_module_babelio_babel_get_max_codepoint(babelio_babel_obj_t* self);
extern uint32_t shared_module_babelio_babel_fetch_glyph_basic_info(babelio_babel_obj_t* self, unichar codepoint);
extern uint16_t shared_module_babelio_babel_fetch_glyph_extended_info(babelio_babel_obj_t* self, unichar codepoint);
extern bool shared_module_babelio_babel_fetch_glyph_data(babelio_babel_obj_t* self, unichar codepoint, _babelio_glyph_t *glyph);
extern unichar shared_module_babelio_babel_uppercase_mapping_for_codepoint(babelio_babel_obj_t* self, unichar codepoint);
extern unichar shared_module_babelio_babel_lowercase_mapping_for_codepoint(babelio_babel_obj_t* self, unichar codepoint);
extern unichar shared_module_babelio_babel_titlecase_mapping_for_codepoint(babelio_babel_obj_t* self, unichar codepoint);
extern void shared_module_babelio_babel_to_uppercase(babelio_babel_obj_t* self, unichar *buf, size_t len);
extern void shared_module_babelio_babel_to_lowercase(babelio_babel_obj_t* self, unichar *buf, size_t len);

#endif // MICROPY_INCLUDED_SHARED_BINDINGS_BABELIO_BABEL_H
