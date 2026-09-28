#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <libmedia/video_frame.h>
#include <libmedia/uvc_demuxing.h>
#include <libmedia/media_demuxing.h>
#include <libmedia/media_packet.h>
#include <libmedia/decode/hw_jpegd_decoder.h>
#include <libmedia/play/fb_video_player.h>
#include <libmedia/utils/file_utils.h>
#include <linux/videodev2.h>
#include <signal.h>

#define message_fifo_path   "/tmp/uvc_message_fifo"

static struct video_player *player = NULL;
static struct media_demuxing *demuxing = NULL;
static struct media_packet *pkt = NULL;
static struct video_frame *frame = NULL;
static struct video_decoder *decoder = NULL;
static pthread_t message_thread;

static volatile int running = 1;

static void *get_message(void *data)
{
    int ret;
    char buffer[128] = {0};
    int fifo_fd = open(message_fifo_path, O_RDONLY);
    if (fifo_fd < 0) {
        fprintf(stderr, "fifo: failed to open fifo for %s\n", message_fifo_path);
        return NULL;
    }

    while (running) {
        ret = read(fifo_fd, buffer, sizeof(buffer));
        if (ret > 0) {
            running = 0;
        } else
            fprintf(stderr, "fifo: read error %s\n", message_fifo_path);
    }

    close(fifo_fd);
    unlink(message_fifo_path);
    return NULL;
}

static void init_get_message(void)
{
    int ret;
    if (access(message_fifo_path, F_OK)) {
        ret = mkfifo(message_fifo_path, 0777);
        if (ret < 0) {
            fprintf(stderr, "fifo: failed to create: %s\n", message_fifo_path);
            return;
        }
    }

    pthread_create(&message_thread, NULL, get_message, NULL);
}

void wait_uvc_node_logo(const char *file_path)
{
    long data_size;
    void *data;
    struct video_frame *frame;
    assert(decoder);
    assert(player);

    int ret = file_read_data(file_path, &data, &data_size);
    if (ret) {
        fprintf(stderr, "failed to read %s data!\n", file_path);
        return;
    }

    struct media_packet pkt = {
        .data = data,
        .size = data_size,
        .type = VIDEO_pkt_mjpeg,
    };

    ret = video_decoder_send_pkt(decoder, &pkt);
    if (ret) {
        fprintf(stderr, "failed to send pkt\n");
        return;
    }

    ret = video_decoder_get_frame(decoder, &frame);
    if (ret) {
        fprintf(stderr, "failed to get frame\n");
        return;
    }
    video_player_set_media_frame(player, frame);
    video_player_display(player);

    video_frame_put(frame);
    free(data);
}

int main(int argc, char *argv[])
{
    int ret;

    if (argc < 3) {
        printf("usage: %s width(uvc video) height(uvc video) device(uvc device) logo_path logo_index\n", argv[0]);
        return -1;
    }

    init_get_message();

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    int logo_index = -1;
    const char *logo_path = NULL;
    if (argc >= 6) {
        logo_path = argv[4];
        logo_index = atoi(argv[5]);
    }
    char *device = "/dev/video0";
    if (argc == 4)
        device = argv[3];

    struct uvc_demuxing_param uvc_demuxing_param = {
        .uvc_format.width = width,
        .uvc_format.height = height,
        .uvc_format.format = V4L2_PIX_FMT_MJPEG,
        .uvc_device = device,
    };
    uvc_demuxing_init_param(&uvc_demuxing_param.param);

    struct hw_jpegd_decoder_param decoder_param = {
        .config.width = width,
        .config.height = height,
        .config.out_fmt = JPEGD_PIX_FMT_NV12,
        .param.codec_name = "mjpg",
        .param.width   = width,
        .param.height  = height,
    };
    hw_jpegd_decoder_init_param(&decoder_param);

    struct fb_video_player_param fb_param = {
        .fb_device = "/dev/fb0",
    };
    fb_video_player_init_param(&fb_param);

    decoder = video_decoder_open(&decoder_param.param);
    if (!decoder) {
        fprintf(stderr, "failed to open video decoder \n");
        video_player_close(player, 1);
        goto err_exit;
    }

    while (access(fb_param.fb_device, F_OK) == -1) {
        if (running == 0)
            goto free_decoder;
        usleep(10000);
    }

    player = video_player_open(&fb_param.param);
    if (!player) {
        fprintf(stderr, "failed to open video player\n");
        goto free_decoder;
    }
    int wait_uvc_count;
    char logo_file_buf[64];

restart:
    wait_uvc_count = 0;
    while (access(uvc_demuxing_param.uvc_device, F_OK) == -1) {
        if (running == 0)
            goto free_player;

        if (logo_index >= 0) {
            sprintf(logo_file_buf, "%s/logo_%02d.jpg",logo_path, wait_uvc_count);
            wait_uvc_node_logo(logo_file_buf);

            wait_uvc_count++;
            if (wait_uvc_count >= logo_index)
                wait_uvc_count = 0;
        } else
            usleep(10000);
    }

    demuxing = demuxing_open(&uvc_demuxing_param.param);
    if (!demuxing) {
        fprintf(stderr, "failed to open video demuxing\n");
        if (running == 0)
            goto free_player;

        usleep(500000);
        goto restart;
    }

    while (running) {
        ret = demuxing_one_pkt(demuxing, &pkt);
        if (ret) {
            fprintf(stderr, "failed to demuxing one pkt \n");
            goto restart_free_demuxing;
        }

        ret = video_decoder_send_pkt(decoder, pkt);
        media_packet_put(pkt);
        if (ret) {
            fprintf(stderr, "failed to send pkt\n");
            goto restart_free_demuxing;
        }

        ret = video_decoder_get_frame(decoder, &frame);
        if (ret) {
            fprintf(stderr, "failed to get frame\n");
            goto restart_free_demuxing;
        }

        video_player_set_media_frame(player, frame);
        video_player_display(player);

        video_frame_put(frame);
    }

free_player:
    video_player_close(player, 1);
free_decoder:
    video_decoder_close(decoder);
err_exit:
    if (demuxing)
        demuxing_close(demuxing);
    pthread_join(message_thread, NULL);
    return -1;

restart_free_demuxing:
    demuxing_close(demuxing);
    demuxing = NULL;

    usleep(300000);
    goto restart;
}