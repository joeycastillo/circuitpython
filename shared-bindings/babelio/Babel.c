#include <stdint.h>
#include <string.h>
#include "lib/utils/context_manager_helpers.h"
#include "py/objproperty.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/runtime0.h"
#include "shared-bindings/babelio/Babel.h"
#include "shared-bindings/util.h"

//| .. currentmodule:: babelio
//|
//| :class:`Babel` -- Universal language support for microcontrollers
//| ====================================================================================
//|
//| Interfaces with an SPI Flash chip containing Unicode data and Unifont glyph bitmaps.
//|
//| .. class:: Babel()
//|
//|   Create an object.
 
STATIC mp_obj_t babelio_babel_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_spi, ARG_cs, ARG_bitmap };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_cs, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_bitmap, MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    babelio_babel_obj_t *self = m_new_obj(babelio_babel_obj_t);
    self->base.type = &babelio_babel_type;
    shared_module_babelio_babel_construct(self, args[ARG_spi].u_obj, args[ARG_cs].u_obj, args[ARG_bitmap].u_obj);
    return MP_OBJ_FROM_PTR(self);
}
 
//|   .. method:: deinit()
//|
//|      Deinitializes Babel and releases any hardware resources for reuse.
//|
STATIC mp_obj_t babelio_babel_deinit(mp_obj_t self_in) {
  shared_module_babelio_babel_deinit(self_in);
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(babelio_babel_deinit_obj, babelio_babel_deinit);
 
//|   .. method:: __enter__()
//|
//|      No-op used by Context Managers.
//|
//  Provided by context manager helper.
 
//|   .. method:: __exit__()
//|
//|      Automatically deinitializes the hardware when exiting a context. See
//|      :ref:`lifetime-and-contextmanagers` for more info.
//|
STATIC mp_obj_t babelio_babel_obj___exit__(size_t n_args, const mp_obj_t *args) {
  shared_module_babelio_babel_deinit(args[0]);
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(babelio_babel___exit___obj, 4, 4, babelio_babel_obj___exit__);







//|   .. attribute:: max_codepoint
//|
//|     The maximum codepoint contained on the SPI Flash chip.
//|
STATIC mp_obj_t babelio_babel_obj_get_max_codepoint(mp_obj_t self_in) {
  return mp_obj_new_int(shared_module_babelio_babel_get_max_codepoint(self_in));
}
MP_DEFINE_CONST_FUN_OBJ_1(babelio_babel_get_max_codepoint_obj, babelio_babel_obj_get_max_codepoint);

const mp_obj_property_t babelio_babel_max_codepoint_obj = {
    .base.type = &mp_type_property,
    .proxy = {(mp_obj_t)&babelio_babel_get_max_codepoint_obj,
              (mp_obj_t)&mp_const_none_obj},
};





STATIC mp_obj_t babelio_babel_obj_str_caseconv(unichar (*op)(babelio_babel_obj_t*, unichar), mp_obj_t self_in, mp_obj_t str_in) {
    GET_STR_DATA_LEN(str_in, self_data, self_len);
    vstr_t vstr;
    vstr_init_len(&vstr, self_len);
    byte *data = (byte*)vstr.buf;
    for (size_t i = 0; i < self_len; i++) {
        *data++ = op(self_in, *self_data++);
    }
    return mp_obj_new_str_from_vstr(mp_obj_get_type(str_in), &vstr);
}

STATIC mp_obj_t babelio_babel_obj_str_lower(mp_obj_t self_in, mp_obj_t str_in) {
    return babelio_babel_obj_str_caseconv(shared_module_babelio_babel_lowercase_mapping_for_codepoint, self_in, str_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(babelio_babel_obj_str_lower_obj, babelio_babel_obj_str_lower);

STATIC mp_obj_t babelio_babel_obj_str_upper(mp_obj_t self_in, mp_obj_t str_in) {
    return babelio_babel_obj_str_caseconv(shared_module_babelio_babel_uppercase_mapping_for_codepoint, self_in, str_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(babelio_babel_obj_str_upper_obj, babelio_babel_obj_str_upper);












STATIC const mp_rom_map_elem_t babelio_babel_locals_dict_table[] = {
    // Methods
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&babelio_babel_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&babelio_babel___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_max_codepoint), MP_ROM_PTR(&babelio_babel_max_codepoint_obj) },
    { MP_ROM_QSTR(MP_QSTR_lowercase), MP_ROM_PTR(&babelio_babel_obj_str_lower_obj) },
    { MP_ROM_QSTR(MP_QSTR_uppercase), MP_ROM_PTR(&babelio_babel_obj_str_upper_obj) },
};
STATIC MP_DEFINE_CONST_DICT(babelio_babel_locals_dict, babelio_babel_locals_dict_table);
 
const mp_obj_type_t babelio_babel_type = {
    { &mp_type_type },
    .name = MP_QSTR_Babel,
    .make_new = babelio_babel_make_new,
    .locals_dict = (mp_obj_dict_t*)&babelio_babel_locals_dict,
};
