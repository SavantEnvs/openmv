// Image input for the QR/barcode/DataMatrix harnesses and the oracle.
//  * fuzz_image_parse(): the fuzzers' input, total over every input of 4+ bytes:
//    [width-1 (u16 LE)][height-1 (u16 LE)][pixels...], short pixel data is zero-padded.
//    The seeds are real OpenMV test images in this layout.
//  * pgm_parse(): strict binary PGM (P5, maxval 255) reader the oracle uses on upstream's test data.
#ifndef PGM_INPUT_H
#define PGM_INPUT_H

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "imlib.h"

#define PGM_MAX_DIM 640

static int pgm_number(const uint8_t *data, size_t size, size_t *pos, int *out) {
    for (;;) {
        while (*pos < size && isspace(data[*pos])) {
            (*pos)++;
        }
        if (*pos < size && data[*pos] == '#') {
            while (*pos < size && data[*pos] != '\n') {
                (*pos)++;
            }
        } else {
            break;
        }
    }
    if (*pos >= size || !isdigit(data[*pos])) {
        return -1;
    }
    int v = 0;
    while (*pos < size && isdigit(data[*pos])) {
        v = v * 10 + (data[*pos] - '0');
        if (v > 100000) {
            return -1;
        }
        (*pos)++;
    }
    *out = v;
    return 0;
}

// Returns 0 and a malloc'd img->data on success, -1 on any malformed input.
static int pgm_parse(const uint8_t *data, size_t size, image_t *img) {
    size_t pos = 2;
    int w, h, maxval;
    if (size < 9 || data[0] != 'P' || data[1] != '5') {
        return -1;
    }
    if (pgm_number(data, size, &pos, &w) || pgm_number(data, size, &pos, &h) ||
        pgm_number(data, size, &pos, &maxval) || maxval != 255) {
        return -1;
    }
    pos++; // single whitespace after maxval
    if (w < 1 || h < 1 || w > PGM_MAX_DIM || h > PGM_MAX_DIM || pos > size ||
        size - pos < (size_t) w * (size_t) h) {
        return -1;
    }
    img->w = w;
    img->h = h;
    img->pixfmt = PIXFORMAT_GRAYSCALE;
    img->data = malloc((size_t) w * h);
    memcpy(img->data, data + pos, (size_t) w * h);
    return 0;
}

// Returns 0 and a malloc'd img->data; -1 only when the 4 header bytes are missing.
static int fuzz_image_parse(const uint8_t *data, size_t size, image_t *img) {
    if (size < 4) {
        return -1;
    }
    int w = 1 + (data[0] | data[1] << 8) % PGM_MAX_DIM;
    int h = 1 + (data[2] | data[3] << 8) % PGM_MAX_DIM;
    size_t pixels = (size_t) w * h;
    size_t avail = size - 4;
    img->w = w;
    img->h = h;
    img->pixfmt = PIXFORMAT_GRAYSCALE;
    img->data = calloc(pixels, 1);
    memcpy(img->data, data + 4, avail < pixels ? avail : pixels);
    return 0;
}

#endif
