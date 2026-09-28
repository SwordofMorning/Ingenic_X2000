#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <libmedia/video_frame.h>

#include <libmedia/video_reader.h>

struct video_reader *video_reader_open(struct video_reader_param *param)
{
    assert(param->cb);
    assert(param->cb->open_reader);

    struct video_rotater *rotater = NULL;
    struct video_reader *reader;

    if (param->rotater_param) {
        rotater = video_rotater_open(param->rotater_param);
        if (!rotater)
            return NULL;
    }

    reader = param->cb->open_reader(param);
    if (!reader) {
        if (rotater)
            video_rotater_close(rotater);

        return NULL;
    }

    reader->is_first = 1;
    reader->rotater = rotater;
    reader->param = *param;

    return reader;
}

void video_reader_close(struct video_reader *reader)
{
    assert(reader->param.cb->close_reader);

    if (reader->rotater)
        video_rotater_close(reader->rotater);

    reader->param.cb->close_reader(reader);
}


int video_reader_drop_video(struct video_reader *reader)
{
    assert(reader->param.cb->drop_video);
    return reader->param.cb->drop_video(reader);
}

int video_reader_read_frame(struct video_reader *reader, struct video_frame **dst_frame)
{
    assert(reader->param.cb->read_video);

    int ret;
    struct video_frame *src_frame = NULL;

    ret = reader->param.cb->read_video(reader, &src_frame);
    if (ret)
        return ret;

    if (reader->rotater) {
        struct video_frame *convert_frame = NULL;
        video_rotater_convert_video_frame(reader->rotater, src_frame, &convert_frame);

        video_frame_put(src_frame);

        *dst_frame = convert_frame;
    } else {
        *dst_frame = src_frame;
    }

    if (reader->is_first) {
        reader->is_first = 0;
        video_frame_put(*dst_frame);
        return 1;
    }

    return ret;
}
