#ifndef _LIBMEDIA_FONT_H_
#define _LIBMEDIA_FONT_H_

#include <libmedia/video_frame.h>

struct font_param;
struct font;

struct font_cb {
    struct font *(*open_font)(struct font_param *param);
    void (*close_font)(struct font *font);
    int (*draw_glyph)(struct font *font, const char *ch, char **next_ch,
                     uint8_t *data, int w, int h, int linesize);
    int (*get_glyph_size)(struct font *font, const char *ch, char **next_ch,
                     int *w, int *h);
    int (*set_pixels_height)(struct font *font, float pixels_height);
    float (*get_pixels_height)(struct font *font);
};

struct font_param {
    const char *path;

    struct font_cb *cb;
};

struct font_glyph_info {
    float ascent;
    float descent;
    float linegap;
};

struct font {
    struct font_param param;
};

void font_set_default_pixels_hight(float pixels_hight);
float font_get_default_pixels_hight(void);

struct font *font_open(struct font_param *param);

void font_close(struct font *font);

int font_get_glyph_char_size(
    struct font *font, const char *ch, char **next_ch, int *w, int *h);

int font_get_glyph_line_size(
    struct font *font, const char *line, char **next_line, int *width, int *height);

int font_get_glyph_str_size(
    struct font *font, const char *str, int *width, int *height);

int font_get_glyph_char_frame(
    struct font *font, const char *ch, char **next_ch, struct video_frame **frame);

int font_get_glyph_line_frame(
    struct font *font, const char *line, char **next_line, struct video_frame **frame);

int font_get_glyph_str_frame(
    struct font *font, const char *line, struct video_frame **frame);

int font_set_pixels_height(struct font *font, float pixels_heihgt);

float font_get_pixels_height(struct font *font);



void font_draw_char(
    struct font *font, char *ch, char **next_ch, int color,
    struct video_frame *dst, int x, int y, int *w_, int *h_);

void font_draw_line(
    struct font *font, char *line, char **next_line, int color,
    struct video_frame *dst, int x, int y, int *w_, int *h_);

void font_draw_str(
    struct font *font, char *str, int color,
    struct video_frame *dst, int x, int y, int *w_, int *h_);

#endif /* _LIBMEDIA_FONT_H_ */
