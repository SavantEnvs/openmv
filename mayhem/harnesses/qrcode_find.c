// Fuzzes imlib_find_qrcodes() (quirc-based QR decoder) on a grayscale image.
#include "pgm_input.h"
#include "collections.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    image_t img = {0};
    if (fuzz_image_parse(data, size, &img)) {
        return -1;
    }

    list_t out;
    list_init(&out, sizeof(find_qrcodes_list_lnk_data_t));
    rectangle_t roi = {0, 0, img.w, img.h};
    imlib_find_qrcodes(&out, &img, &roi);

    while (list_size(&out)) {
        find_qrcodes_list_lnk_data_t r;
        list_pop_front(&out, &r);
        free(r.payload);
    }
    free(img.data);
    return 0;
}
