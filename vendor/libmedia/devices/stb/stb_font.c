#include <stdlib.h>
#include <assert.h>
#include <libmedia/font.h>
#include <libmedia/utils/file_utils.h>

#include <libutils2/list.h>

#include <libmedia/utils/unicode_utils.h>

#include <libmedia/font/stb_font.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct raw_font {
    struct list_head link;

    const char *path;
    int ref_cnt;
    uint8_t *data;
    stbtt_fontinfo info;
};

struct stb_font {
    struct font font;

    struct raw_font *raw;
    float scale;
    float pixels_hight;
};

static LIST_HEAD(list);

static struct raw_font *create_raw_font(const char *font_path)
{
    struct list_head *pos;

    list_for_each(pos, &list) {
        struct raw_font *raw = list_entry(pos, struct raw_font, link);
        if (!strcmp(raw->path, font_path)) {
            raw->ref_cnt++;
            return raw;
        }
    }

    struct raw_font *raw = malloc(sizeof(*raw));
    assert(raw);

    void *font_data;
    if (file_read_data(font_path, &font_data, NULL)) {
        fprintf(stderr, "stb: failed to read %s\n", font_path);
        goto free_raw;
    }

    if (!stbtt_InitFont(&raw->info, font_data, 0)) {
        fprintf(stderr, "stb: failed to init %s\n", font_path);
        goto free_data;
    }

    raw->data = font_data;
    raw->path = strdup(font_path);
    raw->ref_cnt = 1;

    list_add_tail(&raw->link, &list);

    return raw;
free_data:
    free(font_data);
free_raw:
    free(raw);
    return NULL;
}

static void delete_raw_font(struct raw_font *font)
{
    struct list_head *pos;

    list_for_each(pos, &list) {
        struct raw_font *raw = list_entry(pos, struct raw_font, link);
        if (raw == font) {
            raw->ref_cnt--;
            if (raw->ref_cnt == 0) {
                list_del(&raw->link);
                free((void *)raw->path);
                free(raw);
            }
            return;
        }
    }
}

static int stb_set_pixels_height(struct font *font, float pixels_height)
{
    struct stb_font *stb = (void *) font;

    stb->scale = stbtt_ScaleForPixelHeight(&stb->raw->info, pixels_height);
    stb->pixels_hight = pixels_height;

    return 0;
}

static float stb_get_pixels_height(struct font *font)
{
    struct stb_font *stb = (void *) font;

    return stb->pixels_hight;
}

static struct font *stb_open_font(struct font_param *param)
{
    assert(param->path);
    struct raw_font *raw = create_raw_font(param->path);
    if (!raw)
        return NULL;

    struct stb_font *stb = malloc(sizeof(*stb));
    assert(stb);

    stb->raw = raw;
    stb_set_pixels_height(&stb->font, font_get_default_pixels_hight());

    return &stb->font;
}

static void stb_close_font(struct font *font)
{
    struct stb_font *stb = (void *) font;
    delete_raw_font(stb->raw);
    free(stb);
}

static int stb_draw_glyph(struct font *font, const char *ch, char **next_ch,
                    uint8_t *data, int width, int height, int linesize)
{
    struct stb_font *stb = (void *) font;
    float scale = stb->scale;

    int code = utf8_to_unicode(ch, next_ch);
    int index = stbtt_FindGlyphIndex(&stb->raw->info, code);

    /*
     * ascent: 字体从基线到顶部的高度
     * descent: 字体从基线到底部的高度, 负数
     * linegap: 字体行之间的空隙的大小
     * 行间距:ascent - descent + linegap
     */
    int ascent;
    int descent;
    int linegap;

    stbtt_GetFontVMetrics(&stb->raw->info, &ascent, &descent, &linegap);
    int scale_ascent = roundf(ascent*scale);

    int ix0 = 0, iy0 = 0;
    int ix1 = 0, iy1 = 0;

    stbtt_GetGlyphBitmapBox(
        &stb->raw->info, index, scale, scale, &ix0, &iy0, &ix1, &iy1);

    int x = ix0;
    int y = scale_ascent + iy0;
    int w = ix1 - ix0;
    int h = iy1 - iy0;

    if (x+w > width)
        w = width - x;
    if (y+h > height)
        h = height - y;

    if (w <= 0 || h <= 0)
        return 0;

    data = data + y*linesize + x;

    stbtt_MakeGlyphBitmap(
        &stb->raw->info, data, w, h, linesize, scale, scale, index);

    return 0;
}

static int stb_get_glyph_size(struct font *font, const char *ch, char **next_ch,
                    int *w, int *h)
{
    struct stb_font *stb = (void *) font;
    float scale = stb->scale;

    int code = utf8_to_unicode(ch, next_ch);
    int index = stbtt_FindGlyphIndex(&stb->raw->info, code);

    /*
     * ascent: 字体从基线到顶部的高度
     * descent: 字体从基线到底部的高度, 负数
     * linegap: 字体行之间的空隙的大小
     * 行间距:ascent - descent + linegap
     */
    int ascent;
    int descent;
    int linegap;

    stbtt_GetFontVMetrics(&stb->raw->info, &ascent, &descent, &linegap);
    int total_h = roundf((ascent-descent+linegap)*scale);

    int advancewidth = 0;
    int leftsidebearing = 0;

    stbtt_GetGlyphHMetrics(
        &stb->raw->info, index, &advancewidth, &leftsidebearing);

    int total_w = roundf(advancewidth*scale);

    if (w)
        *w = total_w;
    if (h)
        *h = total_h;

    return 0;
}

struct font_cb stb_font_cb = {
    .open_font = stb_open_font,
    .close_font = stb_close_font,
    .draw_glyph = stb_draw_glyph,
    .get_glyph_size = stb_get_glyph_size,
    .set_pixels_height = stb_set_pixels_height,
    .get_pixels_height = stb_get_pixels_height,
};

void stb_font_init_param(struct stb_font_param *p)
{
    p->param.cb = &stb_font_cb;
}
