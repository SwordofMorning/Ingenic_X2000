#include <libmedia/video_display_mode.h>

/*填充 按比例缩放*/
void video_display_adapt_mode(struct video_frame *frame, int display_w, int display_h,
                              struct video_display_data *frame_data)
{
    int src_width = frame->width;
    int src_height = frame->height;

    memset(frame_data, 0, sizeof(*frame_data));

    if (src_width == display_w && src_height == display_h)
        return;

    int scale_w;
    int scale_h;
    int enabled_scale = 0;

    float r1 = 1.0 * src_width / src_height;
    float r2 = 1.0 * display_w / display_h;

    if (r1 > r2) {
        //按宽缩放
        scale_w = display_w;
        scale_h = src_height * display_w / src_width;
        enabled_scale = src_width != scale_w;
    } else {
        //按高缩放
        scale_w = src_width * display_h / src_height;
        scale_h = display_h;
        enabled_scale = src_height != scale_h;
    }

    scale_w = scale_w / 2 * 2;
    scale_h = scale_h / 2 * 2;

    frame_data->scale_enable = enabled_scale;
    frame_data->scale_width = scale_w;
    frame_data->scale_height = scale_h;

    if (display_w > scale_w)
        frame_data->x_offset = (display_w - scale_w) / 2;
    if (display_h > scale_h)
        frame_data->y_offset = (display_h - scale_h) / 2;

    return;
}

/*铺满 裁剪*/
void video_display_paved_mode(struct video_frame *frame, int display_w, int display_h,
                              struct video_display_data *frame_data)
{
    int src_width = frame->width;
    int src_height = frame->height;

    memset(frame_data, 0, sizeof(*frame_data));

    if (src_width == display_w && src_height == display_h)
        return;

    int crop_x;
    int crop_y;

    float r1 = 1.0 * src_width / display_w;
    float r2 = 1.0 * src_height / display_h;

    int p1 = src_width * 100 / src_height;
    int p2 = display_w * 100 / display_h;

    if(r1 < r2) {
        crop_x = display_w * r1;
        crop_y = display_h * r1;
    } else {
        crop_x = display_w * r2;
        crop_y = display_h * r2;
    }

    if (p1 == p2){
        crop_x = src_width;
        crop_y = src_height;
    }

    crop_x = crop_x / 2 * 2;
    crop_y = crop_y / 2 * 2;

    if (crop_x != src_width || crop_y != src_height) {
        frame_data->crop_left = (src_width - crop_x) / 2 / 8 * 8;
        frame_data->crop_top = (src_height - crop_y) / 2 / 8 * 8;

        frame_data->crop_width = crop_x;
        frame_data->crop_height = crop_y;

        frame_data->crop_enable = 1;
    }

    if (crop_x != display_w || crop_y != display_h) {
        frame_data->scale_enable = 1;
        frame_data->scale_width = display_w;
        frame_data->scale_height = display_h;
    }

    return;
}

void video_display_stretch_mode(struct video_frame *frame, int display_w, int display_h,
                                    struct video_display_data *frame_data)
{
    int src_width = frame->width;
    int src_height = frame->height;

    memset(frame_data, 0, sizeof(*frame_data));

    if (src_width == display_w && src_height == display_h)
        return;

    frame_data->scale_enable = 1;
    frame_data->scale_width = display_w;
    frame_data->scale_height = display_h;
}

void video_display_crop_mode(struct video_frame *frame, int display_w, int display_h,
                                 struct video_display_data *frame_data)
{
    int src_width = frame->width;
    int src_height = frame->height;

    memset(frame_data, 0, sizeof(*frame_data));

    if (src_width == display_w && src_height == display_h)
        return;

    int crop_left = 0;
    int crop_top = 0;

    if (src_width > display_w)
        crop_left = (src_width - display_w) / 2;
    if (src_height > display_h)
        crop_top = (src_height - display_h) / 2;

    if (crop_left || crop_top) {
        frame_data->crop_enable = 1;
        frame_data->crop_left = crop_left;
        frame_data->crop_top = crop_top;
        frame_data->crop_width = display_w;
        frame_data->crop_height = display_h;
    }

    if (display_w > src_width)
        frame_data->x_offset = (display_w - src_width) / 2;
    if (display_h > src_height)
        frame_data->y_offset = (display_h - src_height) / 2;
}

void video_display_calculate_data(struct video_frame *frame, struct video_display_config *disp_cfg,
                                  struct video_display_data *frame_data)
{
    int width = disp_cfg->xres;
    int height = disp_cfg->yres;

    switch(disp_cfg->mode) {
        case OUTPUT_ADAPT:
            video_display_adapt_mode(frame, width, height, frame_data);
            break;
        case OUTPUT_PAVED:
            video_display_paved_mode(frame, width, height, frame_data);
            break;
        case OUTPUT_STRETCH:
            video_display_stretch_mode(frame, width, height, frame_data);
            break;
        default:
            video_display_crop_mode(frame, width, height, frame_data);
            break;
    }

    frame_data->x_offset += disp_cfg->xpos;
    frame_data->y_offset += disp_cfg->ypos;

    return;
}

static void crop_frame_free(void *handle, struct video_frame *frame)
{
    struct video_frame *src_frame = frame->pdata;

    video_frame_put(src_frame);
    video_frame_free(frame);
}

static struct video_frame *get_crop_frame(struct video_frame *src_frame, struct video_display_data *frame_data)
{
    if (!frame_data->crop_enable) {
        frame_data->crop_enable = 1;
        frame_data->crop_left = 0;
        frame_data->crop_top = 0;
        frame_data->crop_width = src_frame->width;
        frame_data->crop_height = src_frame->height;
    }

    struct video_frame *crop_frame = video_frame_alloc();
    assert(crop_frame);

    video_frame_crop(src_frame, crop_frame,
                     frame_data->crop_left, frame_data->crop_top,
                     frame_data->crop_width, frame_data->crop_height);

    video_frame_get(crop_frame);
    video_frame_get(src_frame);

    crop_frame->put_frame = crop_frame_free;
    crop_frame->pdata = src_frame;

    return crop_frame;

}

struct video_frame *video_display_get_frame(struct video_frame *src_frame, struct video_scaler_param *scale_param,
                                            struct video_display_config *disp_cfg,struct video_display_data *frame_data)
{
    int ret;
    struct video_frame *scale_frame = NULL;
    struct video_frame *crop_frame = NULL;

    video_display_calculate_data(src_frame, disp_cfg, frame_data);

    crop_frame = get_crop_frame(src_frame, frame_data);

    if (scale_param && frame_data->scale_enable) {
        ret = video_frame_scale(scale_param, crop_frame, &scale_frame, frame_data->scale_width, frame_data->scale_height);
        if (!ret) {
            video_frame_put(crop_frame);
            return scale_frame;
        }
    }

    return crop_frame;
}
