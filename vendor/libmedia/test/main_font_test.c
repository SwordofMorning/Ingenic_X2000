#include <libmedia/font/stb_font.h>
#include <libmedia/utils/file_utils.h>

int main(int argc, char *argv[])
{
    struct stb_font_param param;
    param.param.path = argv[1];

    stb_font_init_param(&param);

    struct font *font = font_open(&param.param);
    if (!font)
        return -1;

    struct video_frame *frame;
    int ret = font_get_glyph_char_frame(font, "A", NULL, &frame);
    if (ret < 0)
        return -1;

    printf("save1: %dx%d\n", frame->linesize[0], frame->height);
    file_write_data("save1", frame->data[0], frame->total_size);

    video_frame_put(frame);

    ret = font_get_glyph_char_frame(font, "你", NULL, &frame);
    if (ret < 0)
        return -1;

    printf("save2: %dx%d\n", frame->linesize[0], frame->height);
    file_write_data("save2", frame->data[0], frame->total_size);

    video_frame_put(frame);

    return 0;
}
