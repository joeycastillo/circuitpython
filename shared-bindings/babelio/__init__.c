#include <stdint.h>
 
#include "py/obj.h"
#include "py/runtime.h"
 
#include "shared-bindings/babelio/__init__.h"
#include "shared-bindings/babelio/Babel.h"
 
STATIC const mp_rom_map_elem_t babelio_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_babelio) },
    { MP_ROM_QSTR(MP_QSTR_Babel), MP_ROM_PTR(&babelio_babel_type) },
};
 
STATIC MP_DEFINE_CONST_DICT(babelio_module_globals, babelio_module_globals_table);
 
const mp_obj_module_t babelio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&babelio_module_globals,
};
