#include <libutils2/nv12_to_rgb.h>

#define range_limit(x) ((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))
#define next_line(addr, len) ((void *)(addr) + (len))

/*计算公式*/
// R = Y + 1.402*(V-128)
// G = Y - 0.34414*(U-128)- 0.71414*(V-128);
// B = Y + 1.772*(U-128)
// 根据计算公式，将除法转换为移位计算
static inline unsigned int yuv_to_r(int y, int v)
{
    int r;

    v = v - 128;
    r = y + v + ((v * 103) >> 8);
    r = range_limit(r);

    return r;
}

static inline unsigned int yuv_to_g(int y, int u, int v)
{
    int g;

    u = u - 128;
    v = v - 128;
    g = y + ((u * 88) >> 8) - ((v * 183) >> 8);
    g = range_limit(g);

    return g;
}

static inline unsigned int yuv_to_b(int y, int u)
{
    int b;

    u = u - 128;
    b = y + u + ((u * 198) >> 8);
    b = range_limit(b);

    return b;
}

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

static inline unsigned short yuv_rgb_565(int y, int u, int v)
{
    unsigned int r = yuv_to_r(y, v);
    unsigned int g = yuv_to_g(y, u, v);
    unsigned int b = yuv_to_b(y ,u);

    return make_rgb565(r, g, b);
}

static inline unsigned int yuv_argb_888(int y, int u, int v)
{
    unsigned int r = yuv_to_r(y, v);
    unsigned int g = yuv_to_g(y, u, v);
    unsigned int b = yuv_to_b(y ,u);

    return make_argb888(r, g, b);
}

//y8是灰度图 仅仅保留Y的值 UV恒定为128
static inline void y8_to_rgb565(void *y_buf, int yuv_line_len, void *rgb_buf, int rgb_line_len, int w, int h)
{
    unsigned char *y = y_buf;
    unsigned short *rgb = rgb_buf;
    unsigned char y0 = 0;
    int i, j;

    for (j = 0; j < h; j ++) {
        for (i = 0; i < w; i ++) {
            y0 = y[i];
            rgb[i] = yuv_rgb_565(y0, 128, 128);
        }
        y = next_line(y, yuv_line_len);
        rgb = next_line(rgb, rgb_line_len);
    }
}

static inline void y8_to_argb888(void *y_buf, int yuv_line_len, void *rgb_buf, int rgb_line_len, int w, int h)
{
    unsigned char *y = y_buf;
    unsigned int *rgb = rgb_buf;
    unsigned char y0 = 0;
    int i, j;

    for (j = 0; j < h; j ++) {
        for (i = 0; i < w; i ++) {
            y0 = y[i];
            rgb[i] = yuv_argb_888(y0, 128, 128);
        }
        y = next_line(y, yuv_line_len);
        rgb = next_line(rgb, rgb_line_len);
    }
}

static inline void nv12_to_rgb565(void *y_buf, void *uv_buf, int yuv_line_len, void *rgb_buf, int rgb_line_len, int w, int h, camera_pixel_fmt data_fmt)
{
    unsigned char *y = y_buf;
    unsigned char *uv = uv_buf;
    unsigned short *rgb = rgb_buf;
    unsigned int y0 = 0;
    unsigned int y1 = 0;
    unsigned int y2 = 0;
    unsigned int y3 = 0;
    unsigned int u  = 0;
    unsigned int v  = 0;
    int i, j;

    for (j = 0; j < h; j += 2) {
        for (i = 0; i < w; i += 2) {
            y0 = y[i + 0];
            y1 = y[i + 1];
            y2 = y[i + yuv_line_len + 0];
            y3 = y[i + yuv_line_len + 1];
            if (data_fmt == CAMERA_PIX_FMT_NV12) {
                u = uv[i + 0];
                v = uv[i + 1];
            } else if (data_fmt == CAMERA_PIX_FMT_NV21) {
                v = uv[i + 0];
                u = uv[i + 1];
            }

            rgb[i + 0] = yuv_rgb_565(y0, u, v);
            rgb[i + 1] = yuv_rgb_565(y1, u, v);
            rgb[i + rgb_line_len / 2] = yuv_rgb_565(y2, u, v);
            rgb[i + rgb_line_len / 2 + 1] = yuv_rgb_565(y3, u, v);
        }
        y = next_line(y, yuv_line_len * 2);
        uv = next_line(uv, yuv_line_len);
        rgb = next_line(rgb, rgb_line_len * 2);
    }
}

static inline void nv12_to_argb888(void *y_buf, void *uv_buf, int yuv_line_len, void *rgb_buf, int rgb_line_len, int w, int h, camera_pixel_fmt data_fmt)
{
    unsigned char *y = y_buf;
    unsigned char *uv = uv_buf;
    unsigned int *rgb = rgb_buf;
    unsigned int y0 = 0;
    unsigned int y1 = 0;
    unsigned int y2 = 0;
    unsigned int y3 = 0;
    unsigned int u  = 0;
    unsigned int v  = 0;
    int i, j;

    for (j = 0; j < h; j += 2) {
        for (i = 0; i < w; i += 2) {
            y0 = y[i + 0];
            y1 = y[i + 1];
            y2 = y[i + yuv_line_len + 0];
            y3 = y[i + yuv_line_len + 1];
            if (data_fmt == CAMERA_PIX_FMT_NV12) {
                u = uv[i + 0];
                v = uv[i + 1];
            } else if (data_fmt == CAMERA_PIX_FMT_NV21) {
                v = uv[i + 0];
                u = uv[i + 1];
            }

            rgb[i + 0] = yuv_argb_888(y0, u, v);
            rgb[i + 1] = yuv_argb_888(y1, u, v);
            rgb[i + rgb_line_len / 4] = yuv_argb_888(y2, u, v);
            rgb[i + rgb_line_len / 4 + 1] = yuv_argb_888(y3, u, v);
        }
        y = next_line(y, yuv_line_len * 2);
        uv = next_line(uv, yuv_line_len);
        rgb = next_line(rgb, rgb_line_len * 2);
    }
}

int nv12_to_rgb(void *cam_y_buf, void *cam_uv_buf, camera_pixel_fmt cam_fmt, int cam_bytes_per_line,
                void *fb_rgb_buf, enum fb_fmt fb_fmt, int fb_bytes_per_line,
                int width, int height)
{
    if(!((cam_fmt == CAMERA_PIX_FMT_NV12) || (cam_fmt == CAMERA_PIX_FMT_NV21))) {
        printf("this camera data_fmt %d is not supported!\n", cam_fmt);
        return -1;
    }

    void *y = cam_y_buf;
    void *uv = cam_uv_buf;
    void *rgb = fb_rgb_buf;

    int yuv_line_len =  cam_bytes_per_line;
    int rgb_line_len =  fb_bytes_per_line;

    switch(fb_fmt)
    {
      case fb_fmt_RGB565:
        nv12_to_rgb565(y, uv, yuv_line_len, rgb, rgb_line_len, width, height, cam_fmt);
        break;
      case fb_fmt_RGB888:
      case fb_fmt_ARGB8888:
        nv12_to_argb888(y, uv, yuv_line_len, rgb, rgb_line_len, width, height, cam_fmt);
        break;
      default:
        printf("this lcd fb_fmt %d is not supported!\n", fb_fmt);
        return -1;
    }
    return 0;
}

int y8_to_rgb(void *cam_y_buf, camera_pixel_fmt cam_fmt, int cam_bytes_per_line,
              void *fb_rgb_buf, enum fb_fmt fb_fmt, int fb_bytes_per_line,
              int width, int height)
{
    if (cam_fmt != CAMERA_PIX_FMT_GREY) {
        printf("this camera data_fmt %d is not supported!\n", cam_fmt);
        return -1;
    }

    void *y = cam_y_buf;
    void *rgb = fb_rgb_buf;

    int yuv_line_len =  cam_bytes_per_line;
    int rgb_line_len =  fb_bytes_per_line;

    switch(fb_fmt)
    {
      case fb_fmt_RGB565:
        y8_to_rgb565(y, yuv_line_len, rgb, rgb_line_len, width, height);
        break;
      case fb_fmt_RGB888:
      case fb_fmt_ARGB8888:
        y8_to_argb888(y, yuv_line_len, rgb, rgb_line_len, width, height);
        break;
      default:
        printf("this lcd fb_fmt %d is not supported!\n", fb_fmt);
        return -1;
    }
    return 0;
}
