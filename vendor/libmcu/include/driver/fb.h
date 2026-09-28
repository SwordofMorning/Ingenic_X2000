#ifndef _FB_H_
#define _FB_H_

#include <soc/lcdc_data.h>

void fb_init(void);
void fb_deinit(void);

int fb_register_lcd(struct lcdc_data *pdata);
void fb_unregister_lcd(struct lcdc_data *pdata);

struct fbdev_data *fb_open(int fbnum);
int fb_enable(struct fbdev_data *fbdev);
int fb_disable(struct fbdev_data *fbdev);
int fb_pan_display(struct fbdev_data *fbdev, int fb_index);

void fb_get_info(struct fbdev_data *fbdev, struct fb_mem_info *info);
int fb_is_enable(struct fbdev_data *fbdev);
unsigned int bytes_per_pixel(enum fb_fmt fmt);

#endif