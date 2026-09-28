#include <libutils2/yuv422_to_rgb.h>

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
    r = y + v + ((v*103) >> 8);
    r = range_limit(r);

    return r;
}

static inline unsigned int yuv_to_g(int y, int u, int v)
{
    int g;

    u = u - 128;
    v = v - 128;
    g = y + ((u*88) >> 8) - ((v*183) >> 8);
    g = range_limit(g);

    return g;
}

static inline unsigned int yuv_to_b(int y, int u)
{
    int b;

    u = u - 128;
    b = y + u + ((u*198) >> 8);
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

void yuv422_to_rgb565(void *yuv_buf, int yuv_line_len, void *rgb_buf, int rgb_line_len, int w, int h)
{
    unsigned char *yuv = yuv_buf;
    unsigned short *rgb = rgb_buf;
    int i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i += 2) {
            unsigned int y0 = yuv[i*2 + 0];
            unsigned int u = yuv[i*2 + 1];
            unsigned int y1 = yuv[i*2 + 2];
            unsigned int v = yuv[i*2 + 3];

            rgb[i + 0] = yuv_rgb_565(y0, u, v);
            rgb[i + 1] = yuv_rgb_565(y1, u, v);
        }

        yuv = next_line(yuv, yuv_line_len);
        rgb = next_line(rgb, rgb_line_len);
    }
}

void yuv422_to_argb888(void *yuv_buf, int yuv_line_len, void *rgb_buf, int rgb_line_len, int w, int h)
{
    unsigned char * yuv = yuv_buf;
    unsigned int * rgb = rgb_buf;
    int i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i += 2) {
            unsigned int y0 = yuv[i*2 + 0];
            unsigned int u = yuv[i*2 + 1];
            unsigned int y1 = yuv[i*2 + 2];
            unsigned int v = yuv[i*2 + 3];

            rgb[i + 0] = yuv_argb_888(y0, u, v);
            rgb[i + 1] = yuv_argb_888(y1, u, v);
        }
        yuv = next_line(yuv, yuv_line_len);
        rgb = next_line(rgb, rgb_line_len);
    }
}

int yuv422_to_rgb(void *cam_yuv_buf, camera_pixel_fmt cam_fmt, int cam_bytes_per_line,
                  void *fb_rgb_buf, enum fb_fmt fb_fmt, int fb_bytes_per_line,
                  int width, int height)
{
    if (cam_fmt != CAMERA_PIX_FMT_YUYV) {
        printf("this camera data_fmt %d is not supported!\n", cam_fmt);
        return -1;
    }

    void *yuv = cam_yuv_buf;
    void *rgb = fb_rgb_buf;

    int yuv_line_len =  cam_bytes_per_line;
    int rgb_line_len =  fb_bytes_per_line;

    switch(fb_fmt)
    {
      case fb_fmt_RGB565:
        yuv422_to_rgb565(yuv, yuv_line_len, rgb, rgb_line_len, width, height);
        break;
      case fb_fmt_RGB888:
      case fb_fmt_ARGB8888:
        yuv422_to_argb888(yuv, yuv_line_len, rgb, rgb_line_len, width, height);
        break;
      default:
        printf("this lcd fb_fmt %d is not supported!\n", fb_fmt);
        return -1;
    }
    return 0;
}
