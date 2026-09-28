#ifndef __VIDEO_DISPLAY_MODE_H__
#define __VIDEO_DISPLAY_MODE_H__

#include <libmedia/video_frame.h>
#include <libmedia/video_scaler.h>

/*视频帧在规定区域内以及规定显示模式下，帧的缩放、裁剪、偏移信息*/
struct video_display_data {
    /*如果缩放和裁剪同时存在,那么先裁剪后缩放*/
    int crop_enable;              /*显示时，是否需要裁剪*/
    int crop_width;               /*裁剪后的宽*/
    int crop_height;              /*裁剪后的高*/
    int crop_left;                /*开始裁剪的水平位置*/
    int crop_top;                 /*开始裁剪的垂直位置*/

    int scale_enable;             /*显示时，是否需要缩放*/
    int scale_width;              /*缩放后的宽*/
    int scale_height;             /*缩放后的高*/

    int x_offset;                 /*缩放、裁剪后，在显示区域的水平偏移*/
    int y_offset;                 /*缩放、裁剪后，在显示区域的垂直偏移*/
};

/*视频帧的显示模式*/
enum video_display_mode {
    OUTPUT_ADAPT,                 /*适应模式，等比例缩放不裁剪，在规定的区域内居中显示*/
    OUTPUT_PAVED,                 /*铺满模式，等比例缩放并裁剪，铺满显示区域*/
    OUTPUT_STRETCH,               /*拉伸模式，直接缩放成规定的显示大小*/
};

/*有效显示区域以及视频在该区域显示模式的配置信息*/
struct video_display_config {
    int xres;                        /*有效显示区域的宽*/
    int yres;                        /*有效显示区域的高*/
    int xpos;                        /*有效显示区域在整个显示区域的水平偏移*/
    int ypos;                        /*有效显示区域在整个显示区域的垂直偏移*/
    enum video_display_mode mode;    /*视频在有效区域的显示模式*/
};

void video_display_adapt_mode(struct video_frame *frame, int display_w, int display_h,
                                  struct video_display_data *frame_data);

void video_display_paved_mode(struct video_frame *frame, int display_w, int display_h,
                                  struct video_display_data *frame_data);

void video_display_stretch_mode(struct video_frame *frame, int display_w, int display_h,
                                    struct video_display_data *frame_data);

void video_display_crop_mode(struct video_frame *frame, int display_w, int display_h,
                                 struct video_display_data *frame_data);

void video_display_calculate_data(struct video_frame *frame, struct video_display_config *disp_cfg,
                                  struct video_display_data *frame_data);

struct video_frame *video_display_get_frame(struct video_frame *src_frame, struct video_scaler_param *scale_param,
                                            struct video_display_config *disp_cfg,struct video_display_data *frame_data);
#endif