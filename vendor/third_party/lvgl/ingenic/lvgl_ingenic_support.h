#ifndef _LVGL_INGENIC_SUPPORT_H_
#define _LVGL_INGENIC_SUPPORT_H_

#ifdef  __cplusplus
extern "C" {
#endif

void lvgl_set_fb_show_frame_rate(int enable);
void lvgl_set_fb_force_frame_cnt(int frame_cnt);
int lvgl_init_fb_display(const char *fb_path);

int lvgl_init_tp_input(const char *tp_path);

void lvgl_usleep_loop(int period_us);
void lvgl_usleep_once(int period_us);

#ifdef  __cplusplus
}
#endif

#endif /* _LVGL_INGENIC_SUPPORT_H_ */
