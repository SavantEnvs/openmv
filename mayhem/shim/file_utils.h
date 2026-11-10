#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "py/obj.h"
typedef struct {
    mp_obj_t fp;
    uint8_t flags;
} file_t;
#define FA_READ             0x01
#define FA_WRITE            0x02
#define FA_OPEN_EXISTING    0x00
#define FA_CREATE_NEW       0x04
#define FA_CREATE_ALWAYS    0x08
#define FA_OPEN_ALWAYS      0x10
#define FA_OPEN_APPEND      0x30
void file_raise_format(file_t *fp);
void file_raise_corrupted(file_t *fp);
void file_raise_error(file_t *fp, mp_rom_error_text_t msg);
void file_open(file_t *fp, const char *path, uint32_t flags);
void file_close(file_t *fp);
void file_seek(file_t *fp, size_t offset);
void file_truncate(file_t *fp);
void file_sync(file_t *fp);
size_t file_tell(file_t *fp);
size_t file_size(file_t *fp);
bool file_eof(file_t *fp);
void file_read(file_t *fp, void *data, size_t size);
void file_write(file_t *fp, const void *data, size_t size);
void file_write_byte(file_t *fp, uint8_t value);
void file_write_short(file_t *fp, uint16_t value);
void file_write_long(file_t *fp, uint32_t value);
void file_read_check(file_t *fp, const void *data, size_t size);
#endif
