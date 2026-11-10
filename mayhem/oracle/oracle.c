// Known-answer checks for the imlib code the harnesses fuzz, built with normal flags against the
// same host shims. Inputs are upstream's own unit-test images and the expected values come from
// scripts/unittest/tests/{qrcodes,datamatrices,barcodes,jpeg_compression,png_compression}.py.
// Every line printed is `key=value`; mayhem/test.sh compares them to the golden values.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgm_input.h"
#include "collections.h"

static uint8_t *slurp(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(2);
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(n);
    if (fread(buf, 1, n, f) != (size_t) n) {
        exit(2);
    }
    fclose(f);
    *size = n;
    return buf;
}

static uint32_t fnv(const uint8_t *p, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

static void load_pgm(const char *dir, const char *name, image_t *img) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    size_t n;
    uint8_t *raw = slurp(path, &n);
    if (pgm_parse(raw, n, img)) {
        fprintf(stderr, "bad pgm %s\n", path);
        exit(2);
    }
    free(raw);
}

static void print_payload(const char *key, const char *p, size_t n) {
    printf("%s=%.*s\n", key, (int) n, p);
}

static void qrcode(const char *dir) {
    image_t img = {0};
    load_pgm(dir, "qrcode.pgm", &img);
    list_t out;
    list_init(&out, sizeof(find_qrcodes_list_lnk_data_t));
    rectangle_t roi = {0, 0, img.w, img.h};
    imlib_find_qrcodes(&out, &img, &roi);
    printf("qr.count=%zu\n", list_size(&out));
    if (list_size(&out)) {
        find_qrcodes_list_lnk_data_t r;
        list_pop_front(&out, &r);
        printf("qr.rect=%d,%d,%d,%d\n", r.rect.x, r.rect.y, r.rect.w, r.rect.h);
        print_payload("qr.payload", r.payload, r.payload_len);
        printf("qr.meta=%d,%d,%d,%d,%u\n", r.version, r.ecc_level, r.mask, r.data_type, r.eci);
    }
}

static void datamatrix(const char *dir) {
    image_t img = {0};
    load_pgm(dir, "datamatrix.pgm", &img);
    list_t out;
    list_init(&out, sizeof(find_datamatrices_list_lnk_data_t));
    rectangle_t roi = {0, 0, img.w, img.h};
    imlib_find_datamatrices(&out, &img, &roi, 200);
    printf("dm.count=%zu\n", list_size(&out));
    if (list_size(&out)) {
        find_datamatrices_list_lnk_data_t r;
        list_pop_front(&out, &r);
        printf("dm.rect=%d,%d,%d,%d\n", r.rect.x, r.rect.y, r.rect.w, r.rect.h);
        print_payload("dm.payload", r.payload, r.payload_len);
        printf("dm.meta=%d,%d,%d,%d,%d\n", r.rotation, r.rows, r.columns, r.capacity, r.padding);
    }
}

static void barcode(const char *dir) {
    image_t img = {0};
    load_pgm(dir, "barcode.pgm", &img);
    list_t out;
    list_init(&out, sizeof(find_barcodes_list_lnk_data_t));
    rectangle_t roi = {0, 0, img.w, img.h};
    imlib_find_barcodes(&out, &img, &roi);
    printf("bc.count=%zu\n", list_size(&out) ? 1 : 0);
    if (list_size(&out)) {
        find_barcodes_list_lnk_data_t r;
        list_pop_front(&out, &r);
        printf("bc.rect=%d,%d,%d,%d\n", r.rect.x, r.rect.y, r.rect.w, r.rect.h);
        print_payload("bc.payload", r.payload, r.payload_len);
        printf("bc.meta=%d,%d,%d\n", r.type, r.rotation, r.quality);
    }
}

static void fill_pattern(image_t *img) {
    uint16_t *px = (uint16_t *) img->data;
    for (int y = 0; y < img->h; y++) {
        for (int x = 0; x < img->w; x++) {
            px[y * img->w + x] = (uint16_t) (((x * 8) << 8) | ((y * 4) << 3) | ((x + y) >> 3));
        }
    }
}

static void jpeg(const char *dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/compressed.jpeg", dir);
    size_t n;
    uint8_t *raw = slurp(path, &n);
    image_t src = {.w = 320, .h = 270, .pixfmt = PIXFORMAT_JPEG, .size = (uint32_t) n, .data = raw};
    printf("jpeg.valid=%d\n", jpeg_is_valid(&src));
    image_t dst = {.w = 320, .h = 270, .pixfmt = PIXFORMAT_RGB565};
    dst.data = calloc(image_size(&dst), 1);
    jpeg_decompress(&dst, &src);
    printf("jpeg.decoded_fnv=%08x\n", fnv(dst.data, image_size(&dst)));
    image_t gray = {.w = 320, .h = 270, .pixfmt = PIXFORMAT_GRAYSCALE};
    gray.data = calloc(image_size(&gray), 1);
    jpeg_decompress(&gray, &src);
    printf("jpeg.gray_fnv=%08x\n", fnv(gray.data, image_size(&gray)));

    image_t pat = {.w = 64, .h = 64, .pixfmt = PIXFORMAT_RGB565};
    pat.data = calloc(image_size(&pat), 1);
    fill_pattern(&pat);
    image_t enc = {.w = 64, .h = 64, .pixfmt = PIXFORMAT_JPEG};
    int rc = jpeg_compress(&pat, &enc, 50, JPEG_SUBSAMPLING_AUTO);
    printf("jpeg.encode_rc=%d\n", rc);
    printf("jpeg.encode_size=%u\n", enc.size);
    printf("jpeg.encode_fnv=%08x\n", fnv(enc.data, enc.size));
    printf("jpeg.encode_valid=%d\n", jpeg_is_valid(&enc));
    image_t back = {.w = 64, .h = 64, .pixfmt = PIXFORMAT_RGB565};
    back.data = calloc(image_size(&back), 1);
    jpeg_decompress(&back, &enc);
    printf("jpeg.roundtrip_fnv=%08x\n", fnv(back.data, image_size(&back)));
}

static void png(void) {
    image_t pat = {.w = 48, .h = 48, .pixfmt = PIXFORMAT_RGB565};
    pat.data = calloc(image_size(&pat), 1);
    fill_pattern(&pat);
    image_t enc = {.w = 48, .h = 48, .pixfmt = PIXFORMAT_PNG};
    int rc = png_compress(&pat, &enc);
    printf("png.encode_rc=%d\n", rc);
    printf("png.signature=%d\n", enc.size > 8 && !memcmp(enc.data, "\x89PNG\r\n\x1a\n", 8));
    printf("png.encode_size=%u\n", enc.size);
    image_t back = {.w = 48, .h = 48, .pixfmt = PIXFORMAT_RGB565};
    back.data = calloc(image_size(&back), 1);
    png_decompress(&back, &enc);
    printf("png.roundtrip_equal=%d\n", !memcmp(back.data, pat.data, image_size(&pat)));
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "scripts/unittest/data";
    qrcode(dir);
    datamatrix(dir);
    barcode(dir);
    jpeg(dir);
    png();
    fflush(stdout);
    return 0;
}
