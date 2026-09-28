#ifndef _VIDEO_READER_H_
#define _VIDEO_READER_H_

#include <libmedia/video_rotater.h>
#include <libmedia/video_frame.h>
#include <libmedia/media_packet.h>

struct video_reader;
struct video_reader_param;

struct video_reader_cb {
    struct video_reader *(*open_reader)(struct video_reader_param *param);
    void (*close_reader)(struct video_reader *reader);
    int (*read_video)(struct video_reader *reader, struct video_frame **frame);
    int (*drop_video)(struct video_reader *reader);
};

struct video_reader_param {
    int width;
    int height;
    enum video_frame_format pixel_fmt;
    int ignore_video_encoder_linesize;
    struct video_reader_cb *cb;
    struct video_rotater_param *rotater_param;
};

struct video_reader {
    struct video_reader_param param;
    struct video_rotater *rotater;
    int is_first;
};

struct video_reader *video_reader_open(struct video_reader_param *param);

void video_reader_close(struct video_reader *reader);

int video_reader_drop_video(struct video_reader *reader);

int video_reader_read_frame(struct video_reader *reader, struct video_frame **dst_frame);

#endif /* _VIDEO_READER_H_ */
