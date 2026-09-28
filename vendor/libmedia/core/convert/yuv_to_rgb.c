#include <stdint.h>
#include <libmedia/yuv_to_rgb.h>

void convert_nv12_to_bgra(void *y, int y_linesize, void *uv, int uv_linesize, void *dst, int dst_linesize, int width, int height)
{
    int i, j;

    for (i = 0; i < height; i+=2) {
        unsigned char *d0 = dst + i*dst_linesize;
        unsigned char *d1 = d0 + dst_linesize;

        unsigned char *Y0 = y + i*y_linesize;
        unsigned char *Y1 = Y0 + y_linesize;
        unsigned char *UV = uv + i*uv_linesize/2;

        for (j = 0; j < width; j += 2) {
            unsigned char y00 = Y0[0];
            unsigned char y01 = Y0[1];
            unsigned char y10 = Y1[0];
            unsigned char y11 = Y1[1];

            unsigned char u00 = UV[0];
            unsigned char v00 = UV[1];

            d0[3] = 0xff;
            d0[2] = to_r(y00, v00);
            d0[1] = to_g(y00, u00, v00);
            d0[0] = to_b(y00, u00);

            d0[3+4] = 0xff;
            d0[2+4] = to_r(y01, v00);
            d0[1+4] = to_g(y01, u00, v00);
            d0[0+4] = to_b(y01, u00);

            d1[3] = 0xff;
            d1[2] = to_r(y10, v00);
            d1[1] = to_g(y10, u00, v00);
            d1[0] = to_b(y10, u00);

            d1[3+4] = 0xff;
            d1[2+4] = to_r(y11, v00);
            d1[1+4] = to_g(y11, u00, v00);
            d1[0+4] = to_b(y11, u00);

            d0 += 8;
            d1 += 8;
            UV += 2;
            Y0 += 2;
            Y1 += 2;
        }
    }
}