#ifndef SHIM_PY_RUNTIME_H
#define SHIM_PY_RUNTIME_H
#include <stdarg.h>
#include "py/obj.h"
#include "py/misc.h"
void mp_raise_msg(const mp_obj_type_t *type, mp_rom_error_text_t msg) __attribute__((noreturn));
void mp_raise_msg_varg(const mp_obj_type_t *type, mp_rom_error_text_t fmt, ...) __attribute__((noreturn));
void mp_raise_ValueError(mp_rom_error_text_t msg) __attribute__((noreturn));
void mp_raise_OSError(int errno_) __attribute__((noreturn));
#define mp_event_handle_nowait() do {} while (0)
#define mp_handle_pending(x) do {} while (0)
#define MP_HANDLE_PENDING_CALLBACKS_ONLY 0
#endif
