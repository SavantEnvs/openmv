#ifndef SHIM_PY_OBJ_H
#define SHIM_PY_OBJ_H
#include <stdint.h>
#include <stddef.h>
typedef void *mp_obj_t;
typedef const char *mp_rom_error_text_t;
typedef struct { int dummy; } mp_obj_type_t;
extern const mp_obj_type_t mp_type_OSError, mp_type_RuntimeError, mp_type_ValueError, mp_type_MemoryError;
#define MP_ERROR_TEXT(s) (s)
#define MP_OBJ_NULL ((mp_obj_t) 0)
#define MP_STATE_PORT(x) x
#endif
