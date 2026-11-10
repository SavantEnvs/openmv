// Fuzzes imlib's baseline JPEG decoder (lib/imlib/jpegd.c), mirroring jpeg_decompress()
// without its mp_raise on error.
#include "../../lib/imlib/jpegd.c"

#define MAX_PIXELS (4 * 1024 * 1024)

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4 || size > (1 << 22)) {
        return -1;
    }

    uint8_t *copy = malloc(size);
    memcpy(copy, data, size);

    image_t src = {.w = 0, .h = 0, .pixfmt = PIXFORMAT_JPEG, .size = (uint32_t) size, .data = copy};
    if (!jpeg_is_valid(&src)) {
        free(copy);
        return -1;
    }

    JPEGIMAGE *jpg = malloc(sizeof(JPEGIMAGE));
    if (!JPEG_openRAM(jpg, copy, (int) size, NULL)) {
        free(jpg);
        free(copy);
        return -1;
    }

    int w = JPEG_getWidth(jpg), h = JPEG_getHeight(jpg);
    if (w <= 0 || h <= 0 || (int64_t) w * h > MAX_PIXELS) {
        free(jpg);
        free(copy);
        return -1;
    }

    static const pixformat_t fmts[] = {PIXFORMAT_BINARY, PIXFORMAT_GRAYSCALE, PIXFORMAT_RGB565};
    pixformat_t fmt = fmts[data[size - 1] % 3];
    switch (fmt) {
        case PIXFORMAT_BINARY: jpg->ucPixelType = ONE_BIT_GRAYSCALE; break;
        case PIXFORMAT_GRAYSCALE: jpg->ucPixelType = EIGHT_BIT_GRAYSCALE; break;
        default: jpg->ucPixelType = RGB565_LITTLE_ENDIAN; break;
    }

    image_t dst = {.w = w, .h = h, .pixfmt = fmt};
    dst.data = calloc(image_size(&dst), 1);
    jpg->pImage = dst.data;
    jpg->pUser = &dst;
    JPEG_decode(jpg, 0, 0, 0);

    free(dst.data);
    free(jpg);
    free(copy);
    return 0;
}
