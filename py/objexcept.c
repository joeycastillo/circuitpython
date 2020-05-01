/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

#include "py/objlist.h"
#include "py/objnamedtuple.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/objtype.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"

#include "supervisor/shared/translate.h"

// Number of items per traceback entry (file, line, block)
#define TRACEBACK_ENTRY_LEN (3)

// Number of traceback entries to reserve in the emergency exception buffer
#define EMG_TRACEBACK_ALLOC (2 * TRACEBACK_ENTRY_LEN)

// Optionally allocated buffer for storing the first argument of an exception
// allocated when the heap is locked.
#if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
#   if MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE > 0
#define mp_emergency_exception_buf_size MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE

void mp_init_emergency_exception_buf(void) {
    // Nothing to do since the buffer was declared statically. We put this
    // definition here so that the calling code can call this function
    // regardless of how its configured (makes the calling code a bit cleaner).
}

#else
#define mp_emergency_exception_buf_size MP_STATE_VM(mp_emergency_exception_buf_size)

void mp_init_emergency_exception_buf(void) {
    mp_emergency_exception_buf_size = 0;
    MP_STATE_VM(mp_emergency_exception_buf) = NULL;
}

mp_obj_t mp_alloc_emergency_exception_buf(mp_obj_t size_in) {
    mp_int_t size = mp_obj_get_int(size_in);
    void *buf = NULL;
    if (size > 0) {
        buf = m_new(byte, size);
    }

    int old_size = mp_emergency_exception_buf_size;
    void *old_buf = MP_STATE_VM(mp_emergency_exception_buf);

    // Update the 2 variables atomically so that an interrupt can't occur
    // between the assignments.
    mp_uint_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    mp_emergency_exception_buf_size = size;
    MP_STATE_VM(mp_emergency_exception_buf) = buf;
    MICROPY_END_ATOMIC_SECTION(atomic_state);

    if (old_buf != NULL) {
        m_del(byte, old_buf, old_size);
    }
    return mp_const_none;
}
#endif
#endif  // MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF

// Instance of GeneratorExit exception - needed by generator.close()
// This would belong to objgenerator.c, but to keep mp_obj_exception_t
// definition module-private so far, have it here.
const mp_obj_exception_t mp_const_GeneratorExit_obj = {{&mp_type_GeneratorExit}, 0, 0, NULL, (mp_obj_tuple_t*)&mp_const_empty_tuple_obj};

void mp_obj_exception_print(const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind) {
    mp_obj_exception_t *o = MP_OBJ_TO_PTR(o_in);
    mp_print_kind_t k = kind & ~PRINT_EXC_SUBCLASS;
    bool is_subclass = kind & PRINT_EXC_SUBCLASS;
    if (!is_subclass && (k == PRINT_REPR || k == PRINT_EXC)) {
        mp_print_str(print, qstr_str(o->base.type->name));
    }

    if (k == PRINT_EXC) {
        mp_print_str(print, ": ");
    }

    if (k == PRINT_STR || k == PRINT_EXC) {
        if (o->args == NULL || o->args->len == 0) {
            mp_print_str(print, "");
            return;
        }
        if (MP_OBJ_IS_SMALL_INT(o->args->items[0]) &&
            mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(o->base.type), MP_OBJ_FROM_PTR(&mp_type_OSError)) &&
            o->args->len <= 2) {
            // try to provide a nice OSError error message
            char decompressed[50];
            const char *msg = mp_common_errno_to_str(o->args->items[0], decompressed, sizeof(decompressed));
            if (msg != NULL) {
                mp_printf(print, "[Errno " INT_FMT "] %s", MP_OBJ_SMALL_INT_VALUE(o->args->items[0]), msg);
                // if second arg exists, it is filename.
                if (o->args->len == 2) {
                    mp_printf(print, ": '%s'", mp_obj_str_get_str(o->args->items[1]));
                }
                return;
            }
        }
        if (o->args->len == 1) {
            mp_obj_print_helper(print, o->args->items[0], PRINT_STR);
            return;
        }
    }
    mp_obj_tuple_print(print, MP_OBJ_FROM_PTR(o->args), kind);
}

mp_obj_t mp_obj_exception_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    mp_arg_check_num(n_args, kw_args, 0, MP_OBJ_FUN_ARGS_MAX, false);

    // Try to allocate memory for the exception, with fallback to emergency exception object
    mp_obj_exception_t *o_exc = m_new_obj_maybe(mp_obj_exception_t);
    if (o_exc == NULL) {
        o_exc = &MP_STATE_VM(mp_emergency_exception_obj);
    }

    // Populate the exception object
    o_exc->base.type = type;
    o_exc->traceback_data = NULL;

    mp_obj_tuple_t *o_tuple;
    if (n_args == 0) {
        // No args, can use the empty tuple straightaway
        o_tuple = (mp_obj_tuple_t*)&mp_const_empty_tuple_obj;
    } else {
        // Try to allocate memory for the tuple containing the args
        o_tuple = m_new_obj_var_maybe(mp_obj_tuple_t, mp_obj_t, n_args);

        #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
        // If we are called by mp_obj_new_exception_msg_varg then it will have
        // reserved room (after the traceback data) for a tuple with 1 element.
        // Otherwise we are free to use the whole buffer after the traceback data.
        if (o_tuple == NULL && mp_emergency_exception_buf_size >=
            EMG_TRACEBACK_ALLOC * sizeof(size_t) + sizeof(mp_obj_tuple_t) + n_args * sizeof(mp_obj_t)) {
            o_tuple = (mp_obj_tuple_t*)
                ((uint8_t*)MP_STATE_VM(mp_emergency_exception_buf) + EMG_TRACEBACK_ALLOC * sizeof(size_t));
        }
        #endif

        if (o_tuple == NULL) {
            // No memory for a tuple, fallback to an empty tuple
            o_tuple = (mp_obj_tuple_t*)&mp_const_empty_tuple_obj;
        } else {
            // Have memory for a tuple so populate it
            o_tuple->base.type = &mp_type_tuple;
            o_tuple->len = n_args;
            memcpy(o_tuple->items, args, n_args * sizeof(mp_obj_t));
        }
    }

    // Store the tuple of args in the exception object
    o_exc->args = o_tuple;

    return MP_OBJ_FROM_PTR(o_exc);
}

// Get exception "value" - that is, first argument, or None
mp_obj_t mp_obj_exception_get_value(mp_obj_t self_in) {
    mp_obj_exception_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->args->len == 0) {
        return mp_const_none;
    } else {
        return self->args->items[0];
    }
}

void mp_obj_exception_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_exception_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        // store/delete attribute
        if (attr == MP_QSTR___traceback__ && dest[1] == mp_const_none) {
            // We allow 'exc.__traceback__ = None' assignment as low-level
            // optimization of pre-allocating exception instance and raising
            // it repeatedly - this avoids memory allocation during raise.
            // However, uPy will keep adding traceback entries to such
            // exception instance, so before throwing it, traceback should
            // be cleared like above.
            self->traceback_len = 0;
            dest[0] = MP_OBJ_NULL; // indicate success
        }
        return;
    }
    if (attr == MP_QSTR_args) {
        dest[0] = MP_OBJ_FROM_PTR(self->args);
    } else if (self->base.type == &mp_type_StopIteration && attr == MP_QSTR_value) {
        dest[0] = mp_obj_exception_get_value(self_in);
    #if MICROPY_CPYTHON_COMPAT
    } else if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(self->base.type), MP_OBJ_FROM_PTR(&mp_type_OSError))) {
        if (attr == MP_QSTR_errno) {
            dest[0] = mp_obj_exception_get_value(self_in);
        } else if (attr == MP_QSTR_strerror) {
            if (self->args->len > 1) {
                dest[0] = self->args->items[1];
            } else if (self->args->len > 0) {
                char decompressed[50];
                const char *msg = mp_common_errno_to_str(self->args->items[0], decompressed, sizeof(decompressed));
                if (msg != NULL) {
                    dest[0] = mp_obj_new_str(msg, strlen(msg));
                } else {
                    dest[0] = mp_const_none;
                }
            } else {
                dest[0] = mp_const_none;
            }
        } else if (attr == MP_QSTR_filename) {
            dest[0] = self->args->len > 2 ? self->args->items[2] : mp_const_none;
        // skip winerror
        } else if (attr == MP_QSTR_filename2) {
            dest[0] = self->args->len > 4 ? self->args->items[4] : mp_const_none;
        }
    #endif
    }
}

const mp_obj_type_t mp_type_BaseException = {
    { &mp_type_type },
    .name = MP_QSTR_BaseException,
    .print = mp_obj_exception_print,
    .make_new = mp_obj_exception_make_new,
    .attr = mp_obj_exception_attr,
};

// List of all exceptions, arranged as in the table at:
// http://docs.python.org/3/library/exceptions.html
MP_DEFINE_EXCEPTION(SystemExit, BaseException)
MP_DEFINE_EXCEPTION(KeyboardInterrupt, BaseException)
MP_DEFINE_EXCEPTION(ReloadException, BaseException)
MP_DEFINE_EXCEPTION(GeneratorExit, BaseException)
MP_DEFINE_EXCEPTION(Exception, BaseException)
  #if MICROPY_PY_ASYNC_AWAIT
  MP_DEFINE_EXCEPTION(StopAsyncIteration, Exception)
  #endif
  MP_DEFINE_EXCEPTION(StopIteration, Exception)
  MP_DEFINE_EXCEPTION(ArithmeticError, Exception)
    //MP_DEFINE_EXCEPTION(FloatingPointError, ArithmeticError)
    MP_DEFINE_EXCEPTION(OverflowError, ArithmeticError)
    MP_DEFINE_EXCEPTION(ZeroDivisionError, ArithmeticError)
  MP_DEFINE_EXCEPTION(AssertionError, Exception)
  MP_DEFINE_EXCEPTION(AttributeError, Exception)
  //MP_DEFINE_EXCEPTION(BufferError, Exception)
  //MP_DEFINE_EXCEPTION(EnvironmentError, Exception) use OSError instead
  MP_DEFINE_EXCEPTION(EOFError, Exception)
  MP_DEFINE_EXCEPTION(ImportError, Exception)
  //MP_DEFINE_EXCEPTION(IOError, Exception) use OSError instead
  MP_DEFINE_EXCEPTION(LookupError, Exception)
    MP_DEFINE_EXCEPTION(IndexError, LookupError)
    MP_DEFINE_EXCEPTION(KeyError, LookupError)
  MP_DEFINE_EXCEPTION(MemoryError, Exception)
  MP_DEFINE_EXCEPTION(NameError, Exception)
    /*
    MP_DEFINE_EXCEPTION(UnboundLocalError, NameError)
    */
  MP_DEFINE_EXCEPTION(OSError, Exception)
  MP_DEFINE_EXCEPTION(TimeoutError, OSError)
    /*
    MP_DEFINE_EXCEPTION(BlockingIOError, OSError)
    MP_DEFINE_EXCEPTION(ChildProcessError, OSError)
    MP_DEFINE_EXCEPTION(ConnectionError, OSError)
      MP_DEFINE_EXCEPTION(BrokenPipeError, ConnectionError)
      MP_DEFINE_EXCEPTION(ConnectionAbortedError, ConnectionError)
      MP_DEFINE_EXCEPTION(ConnectionRefusedError, ConnectionError)
      MP_DEFINE_EXCEPTION(ConnectionResetError, ConnectionError)
    MP_DEFINE_EXCEPTION(InterruptedError, OSError)
    MP_DEFINE_EXCEPTION(IsADirectoryError, OSError)
    MP_DEFINE_EXCEPTION(NotADirectoryError, OSError)
    MP_DEFINE_EXCEPTION(PermissionError, OSError)
    MP_DEFINE_EXCEPTION(ProcessLookupError, OSError)
    MP_DEFINE_EXCEPTION(FileExistsError, OSError)
    MP_DEFINE_EXCEPTION(FileNotFoundError, OSError)
    MP_DEFINE_EXCEPTION(ReferenceError, Exception)
    */
  MP_DEFINE_EXCEPTION(RuntimeError, Exception)
    MP_DEFINE_EXCEPTION(NotImplementedError, RuntimeError)
  MP_DEFINE_EXCEPTION(SyntaxError, Exception)
    MP_DEFINE_EXCEPTION(IndentationError, SyntaxError)
    /*
      MP_DEFINE_EXCEPTION(TabError, IndentationError)
      */
  //MP_DEFINE_EXCEPTION(SystemError, Exception)
  MP_DEFINE_EXCEPTION(TypeError, Exception)
#if MICROPY_EMIT_NATIVE
    MP_DEFINE_EXCEPTION(ViperTypeError, TypeError)
#endif
  MP_DEFINE_EXCEPTION(ValueError, Exception)
#if MICROPY_PY_BUILTINS_STR_UNICODE
    MP_DEFINE_EXCEPTION(UnicodeError, ValueError)
    //TODO: Implement more UnicodeError subclasses which take arguments
#endif
    MP_DEFINE_EXCEPTION(MpyError, ValueError)
  /*
  MP_DEFINE_EXCEPTION(Warning, Exception)
    MP_DEFINE_EXCEPTION(DeprecationWarning, Warning)
    MP_DEFINE_EXCEPTION(PendingDeprecationWarning, Warning)
    MP_DEFINE_EXCEPTION(RuntimeWarning, Warning)
    MP_DEFINE_EXCEPTION(SyntaxWarning, Warning)
    MP_DEFINE_EXCEPTION(UserWarning, Warning)
    MP_DEFINE_EXCEPTION(FutureWarning, Warning)
    MP_DEFINE_EXCEPTION(ImportWarning, Warning)
    MP_DEFINE_EXCEPTION(UnicodeWarning, Warning)
    MP_DEFINE_EXCEPTION(BytesWarning, Warning)
    MP_DEFINE_EXCEPTION(ResourceWarning, Warning)
    */

mp_obj_t mp_obj_new_exception(const mp_obj_type_t *exc_type) {
    return mp_obj_new_exception_args(exc_type, 0, NULL);
}

// "Optimized" version for common(?) case of having 1 exception arg
mp_obj_t mp_obj_new_exception_arg1(const mp_obj_type_t *exc_type, mp_obj_t arg) {
    return mp_obj_new_exception_args(exc_type, 1, &arg);
}

mp_obj_t mp_obj_new_exception_args(const mp_obj_type_t *exc_type, size_t n_args, const mp_obj_t *args) {
    assert(exc_type->make_new == mp_obj_exception_make_new);
    return exc_type->make_new(exc_type, n_args, args, NULL);
}

mp_obj_t mp_obj_new_exception_msg(const mp_obj_type_t *exc_type, const compressed_string_t *msg) {
    return mp_obj_new_exception_msg_varg(exc_type, msg);
}

// The following struct and function implement a simple printer that conservatively
// allocates memory and truncates the output data if no more memory can be obtained.
// It leaves room for a null byte at the end of the buffer.

struct _exc_printer_t {
    bool allow_realloc;
    size_t alloc;
    size_t len;
    byte *buf;
};

STATIC void exc_add_strn(void *data, const char *str, size_t len) {
    struct _exc_printer_t *pr = data;
    if (pr->len + len >= pr->alloc) {
        // Not enough room for data plus a null byte so try to grow the buffer
        if (pr->allow_realloc) {
            size_t new_alloc = pr->alloc + len + 16;
            byte *new_buf = m_renew_maybe(byte, pr->buf, pr->alloc, new_alloc, true);
            if (new_buf == NULL) {
                pr->allow_realloc = false;
                len = pr->alloc - pr->len - 1;
            } else {
                pr->alloc = new_alloc;
                pr->buf = new_buf;
            }
        } else {
            len = pr->alloc - pr->len - 1;
        }
    }
    memcpy(pr->buf + pr->len, str, len);
    pr->len += len;
}


mp_obj_t mp_obj_new_exception_msg_varg(const mp_obj_type_t *exc_type, const compressed_string_t *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    mp_obj_t exception = mp_obj_new_exception_msg_vlist(exc_type, fmt, ap);
    va_end(ap);
    return exception;
}

mp_obj_t mp_obj_new_exception_msg_vlist(const mp_obj_type_t *exc_type, const compressed_string_t *fmt, va_list ap) {
    assert(fmt != NULL);

    // Check that the given type is an exception type
    assert(exc_type->make_new == mp_obj_exception_make_new);

    // Try to allocate memory for the message
    mp_obj_str_t *o_str = m_new_obj_maybe(mp_obj_str_t);
    size_t o_str_alloc = fmt->length + 1;
    byte *o_str_buf = m_new_maybe(byte, o_str_alloc);

    bool used_emg_buf = false;
    #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
    // If memory allocation failed and there is an emergency buffer then try to use
    // that buffer to store the string object and its data (at least 16 bytes for
    // the string data), reserving room at the start for the traceback and 1-tuple.
    if ((o_str == NULL || o_str_buf == NULL)
        && mp_emergency_exception_buf_size >= EMG_TRACEBACK_ALLOC * sizeof(size_t)
            + sizeof(mp_obj_tuple_t) + sizeof(mp_obj_t) + sizeof(mp_obj_str_t) + 16) {
        used_emg_buf = true;
        o_str = (mp_obj_str_t*)((uint8_t*)MP_STATE_VM(mp_emergency_exception_buf)
            + EMG_TRACEBACK_ALLOC * sizeof(size_t) + sizeof(mp_obj_tuple_t) + sizeof(mp_obj_t));
        o_str_buf = (byte*)&o_str[1];
        o_str_alloc = (uint8_t*)MP_STATE_VM(mp_emergency_exception_buf)
            + mp_emergency_exception_buf_size - o_str_buf;
    }
    #endif

    if (o_str == NULL) {
        // No memory for the string object so create the exception with no args
        return mp_obj_exception_make_new(exc_type, 0, 0, NULL);
    }

    if (o_str_buf == NULL) {
        // No memory for the string buffer: the string is compressed so don't add it.
        o_str->len = 0;
        o_str->data = NULL;
    } else {
        // We have some memory to format the string
        struct _exc_printer_t exc_pr = {!used_emg_buf, o_str_alloc, 0, o_str_buf};
        mp_print_t print = {&exc_pr, exc_add_strn};
        char fmt_decompressed[fmt->length];
        decompress(fmt, fmt_decompressed);
        mp_vprintf(&print, fmt_decompressed, ap);
        exc_pr.buf[exc_pr.len] = '\0';
        o_str->len = exc_pr.len;
        o_str->data = exc_pr.buf;
    }

    // Create the string object and call mp_obj_exception_make_new to create the exception
    o_str->base.type = &mp_type_str;
    o_str->hash = qstr_compute_hash(o_str->data, o_str->len);
    mp_obj_t arg = MP_OBJ_FROM_PTR(o_str);
    return mp_obj_exception_make_new(exc_type, 1, &arg, NULL);
}

// return true if the given object is an exception type
bool mp_obj_is_exception_type(mp_obj_t self_in) {
    if (MP_OBJ_IS_TYPE(self_in, &mp_type_type)) {
        // optimisation when self_in is a builtin exception
        mp_obj_type_t *self = MP_OBJ_TO_PTR(self_in);
        if (self->make_new == mp_obj_exception_make_new) {
            return true;
        }
    }
    return mp_obj_is_subclass_fast(self_in, MP_OBJ_FROM_PTR(&mp_type_BaseException));
}

// return true if the given object is an instance of an exception type
bool mp_obj_is_exception_instance(mp_obj_t self_in) {
    return mp_obj_is_exception_type(MP_OBJ_FROM_PTR(mp_obj_get_type(self_in)));
}

// Return true if exception (type or instance) is a subclass of given
// exception type.  Assumes exc_type is a subclass of BaseException, as
// defined by mp_obj_is_exception_type(exc_type).
bool mp_obj_exception_match(mp_obj_t exc, mp_const_obj_t exc_type) {
    // if exc is an instance of an exception, then extract and use its type
    if (mp_obj_is_exception_instance(exc)) {
        exc = MP_OBJ_FROM_PTR(mp_obj_get_type(exc));
    }
    return mp_obj_is_subclass_fast(exc, exc_type);
}

// traceback handling functions

#define GET_NATIVE_EXCEPTION(self, self_in) \
    /* make sure self_in is an exception instance */ \
    assert(mp_obj_is_exception_instance(self_in)); \
    mp_obj_exception_t *self; \
    if (mp_obj_is_native_exception_instance(self_in)) { \
        self = MP_OBJ_TO_PTR(self_in); \
    } else { \
        self = MP_OBJ_TO_PTR(((mp_obj_instance_t*)MP_OBJ_TO_PTR(self_in))->subobj[0]); \
    }

void mp_obj_exception_clear_traceback(mp_obj_t self_in) {
    GET_NATIVE_EXCEPTION(self, self_in);
    // just set the traceback to the null object
    // we don't want to call any memory management functions here
    self->traceback_data = NULL;
}

void mp_obj_exception_add_traceback(mp_obj_t self_in, qstr file, size_t line, qstr block) {
    GET_NATIVE_EXCEPTION(self, self_in);

    // append this traceback info to traceback data
    // if memory allocation fails (eg because gc is locked), just return

    if (self->traceback_data == NULL) {
        self->traceback_data = m_new_maybe(size_t, TRACEBACK_ENTRY_LEN);
        if (self->traceback_data == NULL) {
            #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
            if (mp_emergency_exception_buf_size >= EMG_TRACEBACK_ALLOC * sizeof(size_t)) {
                // There is room in the emergency buffer for traceback data
                size_t *tb = (size_t*)MP_STATE_VM(mp_emergency_exception_buf);
                self->traceback_data = tb;
                self->traceback_alloc = EMG_TRACEBACK_ALLOC;
            } else {
                // Can't allocate and no room in emergency buffer
                return;
            }
            #else
            // Can't allocate
            return;
            #endif
        } else {
            // Allocated the traceback data on the heap
            self->traceback_alloc = TRACEBACK_ENTRY_LEN;
        }
        self->traceback_len = 0;
    } else if (self->traceback_len + TRACEBACK_ENTRY_LEN > self->traceback_alloc) {
        #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
        if (self->traceback_data == (size_t*)MP_STATE_VM(mp_emergency_exception_buf)) {
            // Can't resize the emergency buffer
            return;
        }
        #endif
        // be conservative with growing traceback data
        size_t *tb_data = m_renew_maybe(size_t, self->traceback_data, self->traceback_alloc,
            self->traceback_alloc + TRACEBACK_ENTRY_LEN, true);
        if (tb_data == NULL) {
            return;
        }
        self->traceback_data = tb_data;
        self->traceback_alloc += TRACEBACK_ENTRY_LEN;
    }

    size_t *tb_data = &self->traceback_data[self->traceback_len];
    self->traceback_len += TRACEBACK_ENTRY_LEN;
    tb_data[0] = file;
    tb_data[1] = line;
    tb_data[2] = block;
}

void mp_obj_exception_get_traceback(mp_obj_t self_in, size_t *n, size_t **values) {
    GET_NATIVE_EXCEPTION(self, self_in);

    if (self->traceback_data == NULL) {
        *n = 0;
        *values = NULL;
    } else {
        *n = self->traceback_len;
        *values = self->traceback_data;
    }
}

#if MICROPY_PY_SYS_EXC_INFO
STATIC const mp_obj_namedtuple_type_t code_type_obj = {
    .base = {
        .base = {
            .type = &mp_type_type
        },
        .name = MP_QSTR_code,
        .print = namedtuple_print,
        .make_new = namedtuple_make_new,
        .unary_op = mp_obj_tuple_unary_op,
        .binary_op = mp_obj_tuple_binary_op,
        .attr = namedtuple_attr,
        .subscr = mp_obj_tuple_subscr,
        .getiter = mp_obj_tuple_getiter,
        .parent = &mp_type_tuple,
    },
    .n_fields = 15,
    .fields = {
        MP_QSTR_co_argcount,
        MP_QSTR_co_kwonlyargcount,
        MP_QSTR_co_nlocals,
        MP_QSTR_co_stacksize,
        MP_QSTR_co_flags,
        MP_QSTR_co_code,
        MP_QSTR_co_consts,
        MP_QSTR_co_names,
        MP_QSTR_co_varnames,
        MP_QSTR_co_freevars,
        MP_QSTR_co_cellvars,
        MP_QSTR_co_filename,
        MP_QSTR_co_name,
        MP_QSTR_co_firstlineno,
        MP_QSTR_co_lnotab,
    },
};

STATIC mp_obj_t code_make_new(qstr file, qstr block) {
    mp_obj_t elems[15] = {
        mp_obj_new_int(0),             // co_argcount
        mp_obj_new_int(0),             // co_kwonlyargcount
        mp_obj_new_int(0),             // co_nlocals
        mp_obj_new_int(0),             // co_stacksize
        mp_obj_new_int(0),             // co_flags
        mp_obj_new_bytearray(0, NULL), // co_code
        mp_obj_new_tuple(0, NULL),     // co_consts
        mp_obj_new_tuple(0, NULL),     // co_names
        mp_obj_new_tuple(0, NULL),     // co_varnames
        mp_obj_new_tuple(0, NULL),     // co_freevars
        mp_obj_new_tuple(0, NULL),     // co_cellvars
        MP_OBJ_NEW_QSTR(file),         // co_filename
        MP_OBJ_NEW_QSTR(block),        // co_name
        mp_obj_new_int(1),             // co_firstlineno
        mp_obj_new_bytearray(0, NULL), // co_lnotab
    };

    return namedtuple_make_new((const mp_obj_type_t*)&code_type_obj, 15, elems, NULL);
}

STATIC const mp_obj_namedtuple_type_t frame_type_obj = {
    .base = {
        .base = {
            .type = &mp_type_type
        },
        .name = MP_QSTR_frame,
        .print = namedtuple_print,
        .make_new = namedtuple_make_new,
        .unary_op = mp_obj_tuple_unary_op,
        .binary_op = mp_obj_tuple_binary_op,
        .attr = namedtuple_attr,
        .subscr = mp_obj_tuple_subscr,
        .getiter = mp_obj_tuple_getiter,
        .parent = &mp_type_tuple,
    },
    .n_fields = 8,
    .fields = {
        MP_QSTR_f_back,
        MP_QSTR_f_builtins,
        MP_QSTR_f_code,
        MP_QSTR_f_globals,
        MP_QSTR_f_lasti,
        MP_QSTR_f_lineno,
        MP_QSTR_f_locals,
        MP_QSTR_f_trace,
    },
};

STATIC mp_obj_t frame_make_new(mp_obj_t f_code, int f_lineno) {
    mp_obj_t elems[8] = {
        mp_const_none,             // f_back
        mp_obj_new_dict(0),        // f_builtins
        f_code,                    // f_code
        mp_obj_new_dict(0),        // f_globals
        mp_obj_new_int(0),         // f_lasti
        mp_obj_new_int(f_lineno),  // f_lineno
        mp_obj_new_dict(0),        // f_locals
        mp_const_none,             // f_trace
    };

    return namedtuple_make_new((const mp_obj_type_t*)&frame_type_obj, 8, elems, NULL);
}

STATIC const mp_obj_namedtuple_type_t traceback_type_obj = {
    .base = {
        .base = {
            .type = &mp_type_type
        },
        .name = MP_QSTR_traceback,
        .print = namedtuple_print,
        .make_new = namedtuple_make_new,
        .unary_op = mp_obj_tuple_unary_op,
        .binary_op = mp_obj_tuple_binary_op,
        .attr = namedtuple_attr,
        .subscr = mp_obj_tuple_subscr,
        .getiter = mp_obj_tuple_getiter,
        .parent = &mp_type_tuple,
    },
    .n_fields = 4,
    .fields = {
        MP_QSTR_tb_frame,
        MP_QSTR_tb_lasti,
        MP_QSTR_tb_lineno,
        MP_QSTR_tb_next,
    },
};

STATIC mp_obj_t traceback_from_values(size_t *values, mp_obj_t tb_next) {
    int lineno = values[1];

    mp_obj_t elems[4] = {
        frame_make_new(code_make_new(values[0], values[2]), lineno),
        mp_obj_new_int(0),
        mp_obj_new_int(lineno),
        tb_next,
    };

    return namedtuple_make_new((const mp_obj_type_t*)&traceback_type_obj, 4, elems, NULL);
};

mp_obj_t mp_obj_exception_get_traceback_obj(mp_obj_t self_in) {
    mp_obj_exception_t *self = MP_OBJ_TO_PTR(self_in);

    if (!mp_obj_is_exception_instance(self)) {
        return mp_const_none;
    }

    size_t n, *values;
    mp_obj_exception_get_traceback(self, &n, &values);
    if (n == 0) {
        return mp_const_none;
    }

    mp_obj_t tb_next = mp_const_none;

    for (size_t i = 0; i < n; i += 3) {
        tb_next = traceback_from_values(&values[i], tb_next);
    }

    return tb_next;
}
#endif
