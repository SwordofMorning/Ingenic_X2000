#include <libutils2/simple_bayer16_to_nv12.h>

void simple_bayer16_to_nv12(void *_dst, void *_src, int xres, int yres, int line_length)
{
    int len = line_length * yres;
    unsigned char *src = _src + 1;
    unsigned char *dst = _dst;

    int i = len / 32;
    while (i--) {
        unsigned char s0 = src[0*2];
        unsigned char s1 = src[1*2];
        unsigned char s2 = src[2*2];
        unsigned char s3 = src[3*2];
        unsigned char s4 = src[4*2];
        unsigned char s5 = src[5*2];
        unsigned char s6 = src[6*2];
        unsigned char s7 = src[7*2];
        unsigned char s8 = src[8*2];
        unsigned char s9 = src[9*2];
        unsigned char s10 = src[10*2];
        unsigned char s11 = src[11*2];
        unsigned char s12 = src[12*2];
        unsigned char s13 = src[13*2];
        unsigned char s14 = src[14*2];
        unsigned char s15 = src[15*2];

        dst[0] = s0;
        dst[1] = s1;
        dst[2] = s2;
        dst[3] = s3;
        dst[4] = s4;
        dst[5] = s5;
        dst[6] = s6;
        dst[7] = s7;
        dst[8] = s8;
        dst[9] = s9;
        dst[10] = s10;
        dst[11] = s11;
        dst[12] = s12;
        dst[13] = s13;
        dst[14] = s14;
        dst[15] = s15;

        src += 32;
        dst += 16;
    }

    memset(dst, 0x80, len/4);
}