#include <libmedia/overlay/ffmpeg_video_overlayer.h>
#include <libmedia/utils/file_utils.h>

#define IMG_WIDTH   240
#define IMG_HEIGHT   136

#define MAX_IMG_FILE_NODE_COUNT (1024* 10)
#define MAX_VID_FILE_NODE_COUNT (1024)

#include "turbojpeg.h"

static int decode_jpeg(void *src, int src_size, void *dst, int dst_w, int dst_h)
{
    int ret;
    int width, height;
    int inSubsamp, inColorspace;
    tjhandle tjInstance = NULL;
    tjInstance = tjInitDecompress();
    if (tjInstance == NULL) {
        fprintf(stderr, "initializing decompressor error.\n");
        return -1;
    }

    ret = tjDecompressHeader3(tjInstance, src, src_size, &width, &height, &inSubsamp, &inColorspace);
    if (ret < 0) {
        fprintf(stderr, "reading JPEG header error.\n");
        tjDestroy(tjInstance);
        return -1;
    }

    // printf("%d %d %d %d\n", width, height, inSubsamp, inColorspace);

    ret = tjDecompressToYUV2(
            tjInstance, src, src_size, dst, dst_w, 4, dst_h, 0);
    if (ret < 0) {
        fprintf(stderr, "decompressing JPEG image error.\n");
        tjDestroy(tjInstance);
        return -1;
    }

    tjDestroy(tjInstance);

    return 0;
}

static void *decode_jpeg_and_get_data(void *src, int src_size, int *w, int *h)
{
    int ret;
    int width, height;
    int inSubsamp, inColorspace;
    tjhandle tjInstance = NULL;
    tjInstance = tjInitDecompress();
    if (tjInstance == NULL) {
        fprintf(stderr, "initializing decompressor error.\n");
        return NULL;
    }

    ret = tjDecompressHeader3(tjInstance, src, src_size, &width, &height, &inSubsamp, &inColorspace);
    if (ret < 0) {
        fprintf(stderr, "reading JPEG header error.\n");
        tjDestroy(tjInstance);
        return NULL;
    }

    int dst_size = width * height + width/2 * height/2 + width/2 * height/2;
    void *dst = malloc(dst_size);

    ret = tjDecompressToYUV2(
            tjInstance, src, src_size, dst, width, 4, height, 0);
    if (ret < 0) {
        fprintf(stderr, "decompressing JPEG image error.\n");
        free(dst);
        tjDestroy(tjInstance);
        return NULL;
    }

    *w = width;
    *h = height;

    tjDestroy(tjInstance);

    return dst;
}

static struct video_frame *alloc_bg_frame(int disp_w, int disp_h)
{
    struct video_frame *bg_frame = video_frame_alloc();
    void *buff = malloc(disp_w * disp_h + disp_w/2*disp_h/2 + disp_w/2*disp_h/2);
    bg_frame->width = disp_w;
    bg_frame->height = disp_h;
    bg_frame->data[0] = buff;
    bg_frame->data[1] = (uint8_t *)buff + disp_w * disp_h;
    bg_frame->data[2] = (uint8_t *)buff + disp_w * disp_h + disp_w/2*disp_h/2;
    bg_frame->linesize[0] = disp_w;
    bg_frame->linesize[1] = disp_w/2;
    bg_frame->linesize[2] = disp_w/2;
    bg_frame->format = VIDEO_yuv420p;
    /* 黑色背景 */
    memset(bg_frame->data[0], 0, bg_frame->linesize[0]*bg_frame->height);
    memset(bg_frame->data[1], 128, bg_frame->linesize[1]*bg_frame->height/2);
    memset(bg_frame->data[2], 128, bg_frame->linesize[2]*bg_frame->height/2);

    return bg_frame;
}

static void free_frame(struct video_frame *frame)
{
    free(frame->data[0]);
    video_frame_free(frame);
}

int main_app_picture_viewer(int argc, char *argv[])
{
    int ret = fifo_create_current_pid();
    if (ret < 0)
        return -1;

    struct fifo *fifo = fifo_open_current_pid(1, 0);
    if (!fifo)
        return -1;

    char *s = NULL;

    if (argc >= 3) {
        if (argv[2])
            s = argv[2];
    }

    if (!s) {
        //fprintf(stderr, "No such file or directory %s\n", s); //fix compile error, modify by yhy on 041524
        fprintf(stderr, "argv[2] is invalid, No such file or directory!\n");
        return -1;
    }

    DIR *dir = opendir(s);
    if (!dir) {
        fprintf(stderr, "open directory %s err: %s\n", s, strerror(errno));
        return -1;
    }

    struct dir_table img_file_node_table[MAX_IMG_FILE_NODE_COUNT];

    int file_cnt = read_dir(dir, img_file_node_table, MAX_IMG_FILE_NODE_COUNT, 0);
    if (!file_cnt) {
        fprintf(stderr, "No IMG found.\n");
        return -1;
    }

    int img_width = IMG_WIDTH;
    int img_height = IMG_HEIGHT;
    int display_column = 2;
    int display_rows = 3;

    if (argc >= 7) {
        if (argv[5])
            display_rows = atoi(argv[5]);

        if (argv[6])
            display_column = atoi(argv[6]);
    }

    if (argc >= 5) {
        if (argv[3])
            img_width = atoi(argv[3]);

        if (argv[4])
            img_height = atoi(argv[4]);
    }

    struct video_overlayer_param bg_param;
    bg_param.info.width = img_width*display_column;
    bg_param.info.height = img_height*display_rows;
    bg_param.info.xpos = 0;
    bg_param.info.ypos = 0;
    bg_param.info.format = VIDEO_yuv420p;

    ffmpeg_video_overlayer_init_param(&bg_param);

    struct video_overlayer_param fg_param[display_column*display_rows];
    int i;
    for (i = 0; i < display_column*display_rows; i++) {
        fg_param[i].info.width = img_width;
        fg_param[i].info.height = img_height;
        fg_param[i].info.format = VIDEO_yuv420p;
        fg_param[i].info.xpos = i % display_column * img_width;
        fg_param[i].info.ypos = i / display_column * img_height;
    }

    struct fb_video_player_param fb_param;
    fb_video_player_init_default_param(&fb_param);

    struct video_player *player = video_player_open(&fb_param.param);
    assert(player);

    printf("===============[picture viewer]==================>\n");
    printf("    KEY_DOWN    => quit\n");
    printf("    KEY_LEFT    => last page\n");
    printf("    KEY_RIGHT   => next page\n");
    printf("    KEY_HOME    => large picture display/multi picture display\n");
    printf("=============================================>\n");

    int fg_len = display_column*display_rows;
    int multi_pic_display_cnt = display_column*display_rows;

    int cur_pos = file_cnt - 1;
    int is_display_multi_picture = 1;
    int is_large_pic_mode = 0;
    char buf[32] = {0};

    while (1) {
        ret = fifo_read_pkt(fifo, buf, sizeof(buf), 10);
        if (ret < 0)
            break;

        int cmd_last = 0;
        int cmd_next = 0;
        int cmd_key_home = 0;

        if (buf[0] != '\0') {
            int cmd_quit = 0;
            int key_type = 0;

            pkt_parse_int(buf, "key_type", &key_type, 10);

            if (key_type == KEY_DOWN)
                cmd_quit = 1;

            if (key_type == KEY_LEFT)
                cmd_last = 1;

            if (key_type == KEY_RIGHT)
                cmd_next = 1;

            if (key_type == KEY_HOME)
                cmd_key_home = 1;

            if (cmd_quit)
                break;

            /* 功能键切换浏览模式：大图预览/多图预览 */
            if (cmd_key_home) {
                is_large_pic_mode = !is_large_pic_mode;
                is_display_multi_picture = 1;
                if (is_large_pic_mode) {
                    int pos = cur_pos + multi_pic_display_cnt - 1;
                    if (pos >= file_cnt)
                        pos = 0;
                    cur_pos = pos;
                } else {
                    cur_pos = get_last_pos(cur_pos, file_cnt);
                }
            }
        }

        /* 大图预览模式 */
        if (is_large_pic_mode) {
            /* 左右键切换图片 */
            if (cmd_last)
                cur_pos = get_last_pos(cur_pos, file_cnt);
            else if (cmd_next)
                cur_pos = get_next_pos(cur_pos, file_cnt);

            char file_name[257] = {0};
            sprintf(file_name, "%s/%s", s, img_file_node_table[cur_pos].name);

            long size;
            void *data;

            int ret = file_read_data(file_name, &data, &size);
            if (ret)
                return -1;

            int w = 0;
            int h = 0;

            void *dst_data = decode_jpeg_and_get_data(data, size, &w, &h);

            free(data);

            struct video_frame *src_frame = video_frame_alloc();
            struct video_frame *dst_frame = media_alloter_alloc_video_frame(
                                            NULL, w, h, VIDEO_nv12, 0);

            src_frame->width = w;
            src_frame->height = h;
            src_frame->format = VIDEO_yuv420p;

            src_frame->data[0] = dst_data;
            src_frame->data[1] = (uint8_t *)dst_data + w*h;
            src_frame->data[2] = (uint8_t *)dst_data + w*h + w/2 + h/2;

            src_frame->linesize[0] = w;
            src_frame->linesize[1] = w/2;
            src_frame->linesize[2] = w/2;

            video_frame_copy(src_frame, dst_frame);

            video_player_set_media_frame(player, dst_frame);

            video_player_display(player);

            video_frame_free(src_frame);
            video_frame_put(dst_frame);
            free(dst_data);

            continue;
        }

        /* 多图预览模式 */
        if (cmd_next || cmd_last || is_display_multi_picture) {
            struct video_overlayer *overlayer = video_overlayer_open(&bg_param, fg_param, fg_len);
            assert(overlayer);

            int w = img_width;
            int h = img_height;
            int disp_w = img_width*display_column;
            int disp_h = img_height*display_rows;

            struct video_frame * bg_frame = alloc_bg_frame(disp_w, disp_h);

            ret = video_overlayer_send_bg_frame(overlayer, bg_frame);
            if (ret < 0)
                return -1;

            int i;
            for (i = 0; i < multi_pic_display_cnt; i++) {
                /* 左右键切换图片组 */
                if (cmd_next || is_display_multi_picture)
                    cur_pos = get_next_pos(cur_pos, file_cnt);
                else
                    cur_pos = get_last_pos(cur_pos, file_cnt);

                char file_name[257] = {0};
                sprintf(file_name, "%s/%s", s, img_file_node_table[cur_pos].name);

                long size;
                void *data;

                int dst_size = w*h + w/2*h/2 + w/2*h/2;
                void *dst_data = malloc(dst_size);
                memset(dst_data, 0, dst_size);

                int ret = file_read_data(file_name, &data, &size);
                if (ret)
                    return -1;

                decode_jpeg(data, size, dst_data, w, h);

                free(data);

                struct video_frame *src_frame = video_frame_alloc();
                src_frame->width = w;
                src_frame->height = h;
                src_frame->format = VIDEO_yuv420p;

                src_frame->data[0] = dst_data;
                src_frame->data[1] = (uint8_t *)dst_data + w*h;
                src_frame->data[2] = (uint8_t *)dst_data + w*h + w/2 + h/2;

                src_frame->linesize[0] = w;
                src_frame->linesize[1] = w/2;
                src_frame->linesize[2] = w/2;

                ret = video_overlayer_send_fg_frame(overlayer, src_frame, i);

                video_frame_free(src_frame);
                free(dst_data);
            }

            struct video_frame *frame = NULL;
            ret = video_overlayer_get_frame(overlayer, &frame);

            struct video_frame *display_frame = media_alloter_alloc_video_frame(
                                            NULL, disp_w, disp_h,
                                            VIDEO_nv12, 0);

            video_frame_copy(frame, display_frame);

            video_frame_put(frame);

            video_player_set_media_frame(player, display_frame);

            video_player_display(player);

            video_frame_put(display_frame);

            free_frame(bg_frame);

            video_overlayer_close(overlayer);

            is_display_multi_picture = 0;
        }
    }

    video_player_close(player, 1);

    closedir(dir);

    fifo_write_pkt2(fifo, 1000, "is_quit=1\n");

    printf("picture viewer end\n");

    return 0;
}
