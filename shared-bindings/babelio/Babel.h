#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_BABELIO_BABEL_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_BABELIO_BABEL_H
 
#include "shared-module/babelio/Babel.h"
 
extern const mp_obj_type_t babelio_babel_type;
 
extern void shared_module_babelio_babel_construct(babelio_babel_obj_t* self);
extern void shared_module_babelio_babel_deinit(babelio_babel_obj_t* self);
extern bool shared_module_babelio_babel_deinited(babelio_babel_obj_t* self);
extern mp_int_t shared_module_babelio_babel_get_max_codepoint(babelio_babel_obj_t* self);
 
#endif // MICROPY_INCLUDED_SHARED_BINDINGS_BABELIO_BABEL_H
