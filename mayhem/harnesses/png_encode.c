// Fuzzes imlib's PNG encoder (png_compress() over lodepng) on arbitrary raw pixels, then decodes the result.
// Input: [format][width-1][height-1][pixels...] (short pixel data is zero-padded).
#include "../../lib/imlib/png.c"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 3) {
        return -1;
    }

    static const pixformat_t fmts[] = {PIXFORMAT_BINARY, PIXFORMAT_GRAYSCALE, PIXFORMAT_RGB565};
    image_t src = {.w = data[1] + 1, .h = data[2] + 1, .pixfmt = fmts[data[0] % 3]};

    size_t pixels = image_size(&src);
    src.data = calloc(pixels, 1);
    size_t avail = size - 3;
    memcpy(src.data, data + 3, avail < pixels ? avail : pixels);

    image_t dst = {.w = src.w, .h = src.h, .pixfmt = PIXFORMAT_PNG};
    if (png_compress(&src, &dst) == 0 && dst.data) {
        image_t out = {.w = src.w, .h = src.h, .pixfmt = src.pixfmt};
        out.data = calloc(image_size(&out), 1);
        png_decompress(&out, &dst);
        free(out.data);
        uma_free(dst.data);
    }
    free(src.data);
    return 0;
}
