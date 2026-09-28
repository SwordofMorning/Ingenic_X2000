#ifndef _STB_FONT_H_
#define _STB_FONT_H_

#include <libmedia/font.h>

struct stb_font_param {
    struct font_param param;
};

void stb_font_init_param(struct stb_font_param *p);

#endif /* _STB_FONT_H_ */
