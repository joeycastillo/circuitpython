#include "py/runtime.h"
#include "Babel.h"

void shared_module_babelio_babel_construct(babelio_babel_obj_t* self) {
  self->deinited = 0;
}

bool shared_module_babelio_babel_deinited(babelio_babel_obj_t* self) {
  return self->deinited;
}
 
void shared_module_babelio_babel_deinit(babelio_babel_obj_t* self) {
  self->deinited = 1;
}

mp_int_t shared_module_babelio_babel_get_max_codepoint(babelio_babel_obj_t* self) {
  return 0xFFFF;
}
