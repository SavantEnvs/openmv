// Fuzzes imlib's JPEG encoder (jpege.c: jpeg_compress) on arbitrary raw pixels, then feeds the
// result back through the decoder (jpegd.c).
// Input: [format][quality][subsampling][width-1][height-1][pixels...] (short pixel data is zero-padded).
#include "../../lib/imlib/jpegd.c"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 5) {
        return -1;
    }

    static const pixformat_t fmts[] = {PIXFORMAT_GRAYSCALE, PIXFORMAT_RGB565};
    static const jpeg_subsampling_t subs[] = {JPEG_SUBSAMPLING_AUTO, JPEG_SUBSAMPLING_444,
                                              JPEG_SUBSAMPLING_422, JPEG_SUBSAMPLING_420};
    image_t src = {.w = data[3] + 1, .h = data[4] + 1, .pixfmt = fmts[data[0] & 1]};
    int quality = 1 + data[1] % 100;
    jpeg_subsampling_t sub = subs[data[2] & 3];

    size_t pixels = image_size(&src);
    src.data = calloc(pixels, 1);
    size_t avail = size - 5;
    memcpy(src.data, data + 5, avail < pixels ? avail : pixels);

    image_t dst = {.w = src.w, .h = src.h, .pixfmt = PIXFORMAT_JPEG};
    if (jpeg_compress(&src, &dst, quality, sub) == 0 && dst.data) {
        jpeg_is_valid(&dst);
        JPEGIMAGE *jpg = malloc(sizeof(JPEGIMAGE));
        image_t out = {.w = src.w, .h = src.h, .pixfmt = src.pixfmt};
        out.data = calloc(image_size(&out), 1);
        if (JPEG_openRAM(jpg, dst.data, (int) dst.size, out.data)) {
            jpg->ucPixelType = src.pixfmt == PIXFORMAT_RGB565 ? RGB565_LITTLE_ENDIAN : EIGHT_BIT_GRAYSCALE;
            jpg->pUser = &out;
            JPEG_decode(jpg, 0, 0, 0);
        }
        free(out.data);
        free(jpg);
        uma_free(dst.data);
    }
    free(src.data);
    return 0;
}
