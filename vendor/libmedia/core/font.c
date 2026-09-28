#include <assert.h>
#include <string.h>
#include <libutils2/list.h>
#include <libmedia/video_frame.h>
#include <libmedia/font.h>

static float m_pixels_hight = 64;

void font_set_default_pixels_hight(float pixels_hight)
{
    m_pixels_hight = pixels_hight;
}

float font_get_default_pixels_hight(void)
{
    return m_pixels_hight;
}

struct font *font_open(struct font_param *param)
{
    assert(param->cb);
    assert(param->cb->open_font);

    struct font *font = param->cb->open_font(param);
    font->param = *param;

    return font;
}

void font_close(struct font *font)
{
    assert(font->param.cb->close_font);

    font->param.cb->close_font(font);
}

int font_get_glyph_char_size(
    struct font *font, const char *ch, char **next_ch, int *w, int *h)
{
    assert(font->param.cb->get_glyph_size);

    return font->param.cb->get_glyph_size(font, ch, next_ch, w, h);
}

int font_get_glyph_line_size(
    struct font *font, const char *line, char **next_line, int *width, int *height)
{
    int total_w = 0, total_h = 0;
    int is_empty_line = 1;

    char *l = (void *)line;

    while (1) {
        if (l[0] == '\0')
            break;
        if (l[0] == '\n') {
            l++;
            break;
        }
        if (l[0] == '\r' && l[1] == '\n') {
            l += 2;
            break;
        }

        is_empty_line = 0;

        int w = 0;
        font_get_glyph_char_size(font, l, &l, &w, &total_h);
        total_w += w;
    }

    if (is_empty_line && l[0] != '\0')
        font_get_glyph_char_size(font, " ", NULL, &total_w, &total_h);

    if (width)
        *width = total_w;
    if (height)
        *height = total_h;

    if (next_line)
        *next_line = l;

    if (!total_w || !total_h)
        return -1;

    return 0;
}

int font_get_glyph_str_size(
    struct font *font, const char *str, int *width, int *height)
{
    int max_w = 0, total_h = 0;

    char *l = (void *)str;

    while (1) {
        if (l[0] == '\0')
            break;

        int w = 0, h = 0;
        font_get_glyph_line_size(font, l, &l, &w, &h);
        if (max_w < w)
            max_w = w;
        total_h += h;
    }

    if (width)
        *width = max_w;
    if (height)
        *height = total_h;

    if (!max_w || !total_h)
        return -1;

    return 0;
}

int font_get_glyph_char_frame(
    struct font *font, const char *ch, char **next_ch, struct video_frame **frame)
{
    assert(font->param.cb->draw_glyph);

    int w, h;
    int ret = font_get_glyph_char_size(font, ch, NULL, &w, &h);
    if (ret)
        return ret;

    struct video_frame *f = video_frame_alloc_init(w, h, VIDEO_alpha, 0);
    if (!f)
        return -1;

    memset(f->data[0], 0, f->size[0]);

    ret = font->param.cb->draw_glyph(
        font, ch, next_ch, f->data[0], w, h, f->linesize[0]);
    if (ret)
        video_frame_put(f);
    else
        *frame = f;

    return ret;
}

static void draw_line(
    struct font *font, const char *line, char **next_line,
    uint8_t *data, int width, int height, int linesize, int *line_h)
{
    char *l = (void *)line;

    while (1) {
        if (l[0] == '\0')
            break;
        if (l[0] == '\n') {
            l++;
            break;
        }
        if (l[0] == '\r' && l[1] == '\n') {
            l += 2;
            break;
        }

        int w = 0, h = 0;
        font_get_glyph_char_size(font, l, NULL, &w, &h);
        if (h && line_h)
            *line_h = h;

        font->param.cb->draw_glyph(
            font, l, &l, data, width, height, linesize);

        width -= w;
        data += w;
    }

    if (next_line)
        *next_line = l;
}

int font_get_glyph_line_frame(
    struct font *font, const char *line, char **next_line, struct video_frame **frame)
{
    assert(font->param.cb->draw_glyph);
    int total_w = 0, total_h = 0;

    font_get_glyph_line_size(font, line, NULL, &total_w, &total_h);
    if (!total_w || !total_h)
        return -1;

    struct video_frame *f = video_frame_alloc_init(total_w, total_h, VIDEO_alpha, 0);
    if (!f)
        return -1;

    memset(f->data[0], 0, f->size[0]);

    draw_line(font, line, next_line, f->data[0], total_w, total_h, f->linesize[0], NULL);

    *frame = f;

    return 0;
}

int font_get_glyph_str_frame(
    struct font *font, const char *line, struct video_frame **frame)
{
    assert(font->param.cb->draw_glyph);
    int total_w = 0, total_h = 0;

    font_get_glyph_str_size(font, line, &total_w, &total_h);
    if (!total_w || !total_h)
        return -1;

    struct video_frame *f = video_frame_alloc_init(total_w, total_h, VIDEO_alpha, 0);
    if (!f)
        return -1;

    memset(f->data[0], 0, f->size[0]);

    char *l = (void *)line;
    uint8_t *data = f->data[0];

    while (1) {
        if (l[0] == '\0')
            break;

        int h = 0;
        draw_line(font, l, &l, data, total_w, total_h, f->linesize[0], &h);
        total_h -= h;
        data += h * f->linesize[0];
    }

    *frame = f;

    return 0;
}

int font_set_pixels_height(struct font *font, float pixels_heihgt)
{
    assert(font->param.cb->set_pixels_height);

    return font->param.cb->set_pixels_height(font, pixels_heihgt);
}

float font_get_pixels_height(struct font *font)
{
    assert(font->param.cb->get_pixels_height);

    return font->param.cb->get_pixels_height(font);
}


void font_draw_char(
    struct font *font, char *ch, char **next_ch, int color,
    struct video_frame *dst, int x, int y, int *w_, int *h_)
{
    char *next;
    int w = 0, h = 0;
    if (*ch)
        font_get_glyph_char_size(font, ch, &next, &w, &h);

    if (w_)
        *w_ = w;
    if (h_)
        *h_ = h;

    int ret = video_frame_check_crop_src_dst(
        VIDEO_alpha, w, h, dst->format, dst->width, dst->height, x, y, w, h);
    if (ret) {
        if (next_ch)
            *next_ch = next;
        return;
    }

    struct video_frame *src = NULL;
    ret = font_get_glyph_char_frame(font, ch, next_ch, &src);
    if (ret) {
        fprintf(stderr, "font: failed to draw start char of: %s\n", ch);
        return;
    }

    video_frame_blend_alpha_color(src, color, dst, x, y);

    video_frame_put(src);
}

void font_draw_line(
    struct font *font, char *line, char **next_line, int color,
    struct video_frame *dst, int x, int y, int *w_, int *h_)
{
    int total_w = 0, total_h = 0;
    int is_empty_line = 1;

    while (1) {
        if (line[0] == '\0')
            break;
        if (line[0] == '\n') {
            line++;
            break;
        }
        if (line[0] == '\r' && line[1] == '\n') {
            line += 2;
            break;
        }

        is_empty_line = 0;

        int w;
        font_draw_char(font, line, &line, color, dst, x, y, &w, &total_h);
        total_w += w;
        x += w;
    }

    if (is_empty_line && line[0] != '\0')
        font_get_glyph_char_size(font, "1", NULL, NULL, &total_h);

    if (next_line)
        *next_line = line;

    if (w_)
        *w_ = total_w;
    if (h_)
        *h_ = total_h;
}

void font_draw_str(
    struct font *font, char *str, int color,
    struct video_frame *dst, int x, int y, int *w_, int *h_)
{
    int max_w = 0, total_h = 0;

    while (1) {
        if (str[0] == '\0')
            break;

        int w, h;
        font_draw_line(font, str, &str, color, dst, x, y, &w, &h);
        if (max_w < w)
            max_w = w;
        y += h;
        total_h += h;
    }

    if (w_)
        *w_ = max_w;
    if (h_)
        *h_ = total_h;
}
