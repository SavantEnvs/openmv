// Fuzzes imlib's PNG decoding (png.c over lodepng), mirroring png_decompress() without its mp_raise on error.
#include "../../lib/imlib/png.c"

#define MAX_PIXELS (4 * 1024 * 1024)

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 9) {
        return -1;
    }
    size--;

    static const pixformat_t fmts[] = {PIXFORMAT_BINARY, PIXFORMAT_GRAYSCALE, PIXFORMAT_RGB565};
    pixformat_t fmt = fmts[data[size] % 3];

    LodePNGState state;
    lodepng_state_init(&state);
    unsigned w = 0, h = 0;
    if (lodepng_inspect(&w, &h, &state, data, size) || !w || !h || (uint64_t) w * h > MAX_PIXELS) {
        lodepng_state_cleanup(&state);
        return -1;
    }
    lodepng_state_cleanup(&state);

    lodepng_state_init(&state);
    state.lodepng_convert = &lodepng_convert_cb;
    switch (fmt) {
        case PIXFORMAT_BINARY:
            state.info_raw.bitdepth = 8;
            state.info_raw.colortype = LCT_CUSTOM;
            state.info_raw.customfmt = PIXFORMAT_BINARY;
            break;
        case PIXFORMAT_GRAYSCALE:
            state.info_raw.bitdepth = 8;
            state.info_raw.colortype = LCT_CUSTOM;
            state.info_raw.customfmt = PIXFORMAT_GRAYSCALE;
            break;
        default:
            state.info_raw.bitdepth = 16;
            state.info_raw.colortype = LCT_CUSTOM;
            state.info_raw.customfmt = PIXFORMAT_RGB565;
            break;
    }

    uint8_t *png_data = NULL;
    unsigned error = lodepng_decode(&png_data, &w, &h, &state, data, size);
    lodepng_state_cleanup(&state);
    if (!error) {
        image_t img = {.w = w, .h = h, .pixfmt = fmt};
        volatile uint8_t sink = 0;
        size_t n = image_size(&img);
        for (size_t i = 0; i < n; i += 64) {
            sink ^= png_data[i];
        }
        (void) sink;
    }
    uma_free(png_data);
    return error ? -1 : 0;
}
