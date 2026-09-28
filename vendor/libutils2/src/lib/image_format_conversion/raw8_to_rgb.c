#include <libutils2/raw8_to_rgb.h>

#define range_limit(x) ((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))
#define G_SCALE 0.9
#define G_SHIFT_CAL (int)(G_SCALE * 256) >> 8
#define next_line(addr, len) ((void *)(addr) + (len))

/*                      raw8                     */
//     0 0 0 0 0        0 a a a 0       a a a a a
//     0 a a a 0        0 a a a 0       a a a a a
//     0 d x b 0 ---->  0 d x b 0 ----> d d x b b
//     0 c c c 0        0 c c c 0       c c c c c
//     0 0 0 0 0        0 c c c 0       c c c c c
//   边缘处理：将边缘内一行a的像素内存复制给边缘b的内存
//   其余非边缘点 使用插值法计算
static inline unsigned short make_rgb565(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned short rgb = (((int)r & 0xf8) << 8) | (((int)g & 0xfc) << 3) | (((int)b & 0xf8) >> 3);
    return rgb;
}

static inline unsigned int make_argb888(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned int rgb = (0xff << 24) | ((int)r << 16) | ((int)g << 8) | (int)b;
    return rgb;
}

static inline unsigned char avg_2(unsigned int x0, unsigned int x1)
{
    unsigned char avg = (x0 + x1) / 2;
    return avg;
}

static inline unsigned char avg_4(unsigned int x0, unsigned int x1, unsigned int x2, unsigned int x3)
{
    unsigned char avg = (x0 + x1 + x2 + x3) / 4;
    return avg;
}

static inline unsigned char avg_5(unsigned int x0, unsigned int x1, unsigned int x2, unsigned int x3, unsigned int x4)
{
    unsigned char avg = (x0 + x1 + x2 + x3 + x4) / 5;
    return avg;
}

static inline unsigned char green_down_scale(unsigned char g)
{
    unsigned char new_g = (int)g * G_SHIFT_CAL;
    return new_g;
}

//拷贝列
static inline void copy_565_row(void *rgb_buf, int line_len, int w, int h)
{
    int i;
    unsigned short *c0 = rgb_buf;
    unsigned short *c1 = c0 + (w-1);

    for (i = 0; i < h; i++)
    {
        c0[0] = c0[1];
        c1[0] = c1[-1];

        c0 = next_line(c0, line_len);
        c1 = next_line(c1, line_len);
    }
}

static inline void copy_888_row(void *rgb_buf, int line_len, int w, int h)
{
    int i;
    unsigned int *c0 = rgb_buf;
    unsigned int *c1 = c0 + (w-1);

    for (i = 0; i < h; i++)
    {
        c0[0] = c0[1];
        c1[0] = c1[-1];

        c0 = next_line(c0, line_len);
        c1 = next_line(c1, line_len);
    }
}

static inline void copy_edges(void *rgb_buf, int line_len, int w, int h, enum fb_fmt fb_format)
{
    /* 复制首行(l0)、尾行(l1)
    */
    void *l0 = rgb_buf;
    void *l1 = rgb_buf + line_len*(h-1);

    memcpy(l0, l0+line_len, line_len);
    memcpy(l1, l1-line_len, line_len);

    /* 复制左右两列, 一次复制一行中的左(c0)、右(c1)边缘两个点
    */
   if(fb_format == fb_fmt_RGB565) copy_565_row(rgb_buf, line_len, w, h);
   else copy_888_row(rgb_buf, line_len, w, h);
}

// r g r     r0 g0 r1
// g b g --> g1 b0 g2
// r g r     r2 g3 r3
// 元素b，插值计算r、g
static inline unsigned int b_to_rgb(void *pixel, int line_len, enum fb_fmt fb_format)
{
    unsigned char *p0 = pixel - line_len;
    unsigned char *p1 = pixel;
    unsigned char *p2 = pixel + line_len;

    unsigned char r0 = p0[-1], g0 = p0[0], r1 = p0[1];
    unsigned char g1 = p1[-1], b0 = p1[0], g2 = p1[1];
    unsigned char r2 = p2[-1], g3 = p2[0], r3 = p2[1];

    /* b0 不做均值计算，因为在3*3的矩阵范围内只有唯一的b0 */
    unsigned char b = b0;
    unsigned char g = avg_4(g0, g1, g2, g3);
    unsigned char r = avg_4(r0, r1, r2, r3);

    g = green_down_scale(g);

    if(fb_format == fb_fmt_RGB565) return make_rgb565(r, g, b);
    return make_argb888(r, g, b);
}

// b g b     b0 g0 b1
// g r g --> g1 r0 g2
// b g b     b2 g3 b3
// 元素r，插值计算b、g
static inline unsigned int r_to_rgb(void *pixel, int line_len, enum fb_fmt fb_format)
{
    unsigned char *p0 = pixel - line_len;
    unsigned char *p1 = pixel;
    unsigned char *p2 = pixel + line_len;

    unsigned char b0 = p0[-1], g0 = p0[0], b1 = p0[1];
    unsigned char g1 = p1[-1], r0 = p1[0], g2 = p1[1];
    unsigned char b2 = p2[-1], g3 = p2[0], b3 = p2[1];

    /* r0 不做均值计算，因为在3*3的矩阵范围内只有唯一的r0 */
    unsigned char r = r0;
    unsigned char g = avg_4(g0, g1, g2, g3);
    unsigned char b = avg_4(b0, b1, b2, b3);

    g = green_down_scale(g);

    if(fb_format == fb_fmt_RGB565) return make_rgb565(r, g, b);
    return make_argb888(r, g, b);
}

// g r g     g1 r0 g2
// b g b --> b0 g0 b1
// g r g     g3 r1 g4
// 元素g，插值计算b、g、r
static inline unsigned int g_to_rgb1(void *pixel, int line_len, enum fb_fmt fb_format)
{
    unsigned char *p0 = pixel - line_len;
    unsigned char *p1 = pixel;
    unsigned char *p2 = pixel + line_len;

    unsigned char g1 = p0[-1], b0 = p0[0], g2 = p0[1];
    unsigned char r0 = p1[-1], g0 = p1[0], r1 = p1[1];
    unsigned char g3 = p2[-1], b1 = p2[0], g4 = p2[1];

    unsigned char r = avg_2(r0, r1);
    unsigned char b = avg_2(b0, b1);
    unsigned char g = avg_5(g0, g1, g2, g3, g4);

    g = green_down_scale(g);

    if(fb_format == fb_fmt_RGB565) return make_rgb565(r, g, b);
    return make_argb888(r, g, b);
}

// g b g     g1 b0 g2
// r g r --> r0 g0 r1
// g b g     g3 b1 g4
// 元素g，插值计算b、g、r
static inline unsigned int g_to_rgb2(void *pixel, int line_len, enum fb_fmt fb_format)
{
    unsigned char *p0 = pixel - line_len;
    unsigned char *p1 = pixel;
    unsigned char *p2 = pixel + line_len;

    unsigned char g1 = p0[-1], b0 = p0[0], g2 = p0[1];
    unsigned char r0 = p1[-1], g0 = p1[0], r1 = p1[1];
    unsigned char g3 = p2[-1], b1 = p2[0], g4 = p2[1];

    unsigned char r = avg_2(r0, r1);
    unsigned char b = avg_2(b0, b1);
    unsigned char g = avg_5(g0, g1, g2, g3, g4);

    g = green_down_scale(g);

    if(fb_format == fb_fmt_RGB565) return make_rgb565(r, g, b);
    return make_argb888(r, g, b);
}

static inline void rggb_to_rgb565(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned short *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = b_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = g_to_rgb1(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb2(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = r_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
    copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB565);
}

static inline void rggb_to_argb888(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned int *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = b_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = g_to_rgb1(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb2(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = r_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
    copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB888);
}

static inline void gbrg_to_rgb565(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned short *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb2(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = r_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = b_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = g_to_rgb1(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
   copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB565);
}

static inline void gbrg_to_argb888(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned int *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb2(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = r_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = b_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = g_to_rgb1(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
   copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB888);
}

static inline void grbg_to_rgb565(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned short *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb1(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = b_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = r_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = g_to_rgb2(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
    copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB565);
}

static inline void grbg_to_argb888(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned int *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb1(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = b_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = r_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = g_to_rgb2(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
    copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB888);
}

static inline void bggr_to_rgb565(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned short *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = r_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = g_to_rgb2(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb1(bayer+i, bayer_line_len, fb_fmt_RGB565);
            rgb[i+1] = b_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB565);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
    copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB565);
}

static inline void bggr_to_argb888(void *from_bayer, int bayer_line_len, void *to_rgb, int rgb_line_len, int w, int h)
{
    unsigned char *bayer = from_bayer;
    unsigned int *rgb = to_rgb;
    int i, j;

    /* 起始地址偏移，保留四边边缘的图像，另作边缘处理 */
    rgb = next_line(rgb+1, rgb_line_len);
    bayer = next_line(bayer+1, bayer_line_len);

    for (j = 0; j < h-2; j += 2) {
        for (i = 0; i < w-2; i += 2) {
            rgb[i] = r_to_rgb(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = g_to_rgb2(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);

        for (i = 0; i < w-2; i += 2) {
            rgb[i] = g_to_rgb1(bayer+i, bayer_line_len, fb_fmt_RGB888);
            rgb[i+1] = b_to_rgb(bayer+i+1, bayer_line_len, fb_fmt_RGB888);
        }
        rgb = next_line(rgb, rgb_line_len);
        bayer = next_line(bayer, bayer_line_len);
    }
    copy_edges(to_rgb, rgb_line_len, w, h, fb_fmt_RGB888);
}

int raw8_to_rgb(void *cam_raw_buf, camera_pixel_fmt cam_fmt, int cam_bytes_per_line,
                void *fb_rgb_buf, enum fb_fmt fb_fmt, int fb_bytes_per_line,
                int width, int height)
{
    void *y = cam_raw_buf;
    void *rgb = fb_rgb_buf;

    int yuv_line_len = cam_bytes_per_line;
    int rgb_line_len = fb_bytes_per_line;

    switch(fb_fmt)
    {
      case fb_fmt_RGB565:
        switch (cam_fmt)
        {
          case CAMERA_PIX_FMT_SBGGR8:
            bggr_to_rgb565(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          case CAMERA_PIX_FMT_SGBRG8:
            gbrg_to_rgb565(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          case CAMERA_PIX_FMT_SGRBG8:
            grbg_to_rgb565(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          case CAMERA_PIX_FMT_SRGGB8:
            rggb_to_rgb565(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          default:
            printf("this camera data_fmt %d is not supported!\n", cam_fmt);
            return -1;
        }
        break;
      case fb_fmt_RGB888:
      case fb_fmt_ARGB8888:
        switch (cam_fmt)
        {
          case CAMERA_PIX_FMT_SBGGR8:
            bggr_to_argb888(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          case CAMERA_PIX_FMT_SGBRG8:
            gbrg_to_argb888(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          case CAMERA_PIX_FMT_SGRBG8:
            grbg_to_argb888(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          case CAMERA_PIX_FMT_SRGGB8:
            rggb_to_argb888(y, yuv_line_len, rgb, rgb_line_len, width, height);
            break;
          default:
            printf("this camera data_fmt %d is not supported!\n", cam_fmt);
            return -1;
        }
        break;
      default:
        printf("this lcd fb_fmt %d is not supported!\n", fb_fmt);
        return -1;
    }
    return 0;
}
