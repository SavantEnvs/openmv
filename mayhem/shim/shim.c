// Host stand-ins for the MicroPython/firmware services imlib calls.
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "umalloc.h"
#include "file_utils.h"

const mp_obj_type_t mp_type_OSError, mp_type_RuntimeError, mp_type_ValueError, mp_type_MemoryError;

void m_free(void *p) {
    free(p);
}

// A Python exception in the firmware; on the host it is a hard stop.
void mp_raise_msg(const mp_obj_type_t *type, mp_rom_error_text_t msg) {
    (void) type;
    fprintf(stderr, "mp_raise: %s\n", msg ? msg : "");
    abort();
}

void mp_raise_msg_varg(const mp_obj_type_t *type, mp_rom_error_text_t fmt, ...) {
    (void) type;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "mp_raise: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    abort();
}

void mp_raise_ValueError(mp_rom_error_text_t msg) {
    mp_raise_msg(&mp_type_ValueError, msg);
}

void mp_raise_OSError(int errno_) {
    fprintf(stderr, "mp_raise: OSError %d\n", errno_);
    abort();
}

#define UMA_AVAIL_BYTES (256 * 1024)

void uma_fail(void) {
    fprintf(stderr, "uma_fail\n");
    abort();
}

void *uma_malloc(size_t size, uint32_t flags) {
    (void) flags;
    return malloc(size ? size : 1);
}

void *uma_malign(size_t size, size_t align, uint32_t flags) {
    (void) flags;
    void *p = NULL;
    if (posix_memalign(&p, align < sizeof(void *) ? sizeof(void *) : align, size ? size : 1)) {
        return NULL;
    }
    return p;
}

void *uma_calloc(size_t size, uint32_t flags) {
    (void) flags;
    return calloc(1, size ? size : 1);
}

void *uma_realloc(void *ptr, size_t size, uint32_t flags) {
    (void) flags;
    return realloc(ptr, size ? size : 1);
}

void uma_free(void *ptr) {
    free(ptr);
}

void uma_collect(void) {
}

void uma_collect_lock(void) {
}

void uma_collect_unlock(void) {
}

size_t uma_avail(uint32_t flags) {
    (void) flags;
    return UMA_AVAIL_BYTES;
}

void uma_transient_acquire(void) {
}

void uma_transient_release(void) {
}

// File I/O goes through the MicroPython VFS in the firmware; no harness opens files.
static void no_files(void) {
    fprintf(stderr, "file I/O is not available in the host build\n");
    abort();
}

void file_raise_format(file_t *fp) { (void) fp; no_files(); }
void file_raise_corrupted(file_t *fp) { (void) fp; no_files(); }
void file_raise_error(file_t *fp, mp_rom_error_text_t msg) { (void) fp; (void) msg; no_files(); }
void file_open(file_t *fp, const char *path, uint32_t flags) { (void) fp; (void) path; (void) flags; no_files(); }
void file_close(file_t *fp) { (void) fp; no_files(); }
void file_seek(file_t *fp, size_t offset) { (void) fp; (void) offset; no_files(); }
void file_truncate(file_t *fp) { (void) fp; no_files(); }
void file_sync(file_t *fp) { (void) fp; no_files(); }
size_t file_tell(file_t *fp) { (void) fp; no_files(); return 0; }
size_t file_size(file_t *fp) { (void) fp; no_files(); return 0; }
bool file_eof(file_t *fp) { (void) fp; no_files(); return true; }
void file_read(file_t *fp, void *data, size_t size) { (void) fp; (void) data; (void) size; no_files(); }
void file_write(file_t *fp, const void *data, size_t size) { (void) fp; (void) data; (void) size; no_files(); }
void file_write_byte(file_t *fp, uint8_t value) { (void) fp; (void) value; no_files(); }
void file_write_short(file_t *fp, uint16_t value) { (void) fp; (void) value; no_files(); }
void file_write_long(file_t *fp, uint32_t value) { (void) fp; (void) value; no_files(); }
void file_read_check(file_t *fp, const void *data, size_t size) { (void) fp; (void) data; (void) size; no_files(); }
