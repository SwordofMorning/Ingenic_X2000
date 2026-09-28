#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include <libhardware2/dbox.h>
#include <libhardware2/rmem.h>

#define ALIGN_DOWN(n, align)            (n & (~(align - 1)))

#define MAY_UNUSED(n)                   ((n)=(n))

#define NAME_END                        (NULL)
#define INDEX_END                       (-1)
#define INFO_END                        {NAME_END, INDEX_END}

typedef struct {
    int fd;
    int size;
    void *mmap_addr;
    unsigned long phy_addr;
}rmem_info_t;

typedef struct {
    const char *name;
    int index;
}parse_info_t;

static const parse_info_t g_mode_info[] = {
    { "draw_rectangle",       DBOX_MODE_RECT},
    { "draw_range",           DBOX_MODE_RANGE},
    { "draw_horizontal_line", DBOX_MODE_HORI},
    { "draw_vertical_line",   DBOX_MODE_VERT},
    INFO_END,
};

static const parse_info_t g_color_info[] = {
    { "red",    DBOX_COLOR_RED},
    { "black",  DBOX_COLOR_BLACK},
    { "green",  DBOX_COLOR_GREEN},
    { "yellow", DBOX_COLOR_YELLOW},
    INFO_END,
};

static void cmd_usage(const char *command)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "Usage:%s <mode=drawing_type> <background_image=background_image_path> <is_rgba=value>\n" \
                     "<background_width=value> <background_height=value> <output_file=output_file_path>\n" \
                     "<start_point_x=value> <start_point_y=value> <box_width=value> <box_height=value>\n" \
                     "<line_width=value> <line_lenght=value> <color=value>\n", command);

    fprintf(stderr, "\nArguments as follow\n"
                    "<mode>                  The drawing modes are draw_rectangle, draw_range, draw_horizontal_line\n" \
                    "                        and draw_vertical_line\n");

    fprintf(stderr, "<background_image>      Specifies the path to draw the background image\n");

    fprintf(stderr, "<is_rgba>               Enter whether the image format is rgba.\n" \
                    "                        Note: Only nv12 formats are supported currently\n");

    fprintf(stderr, "<background_width>      The width of the background image. Note max:65535\n");
    fprintf(stderr, "<background_height>     The height of the background image. Note max:65535\n");

    fprintf(stderr, "<output_file>           Output file path after processing\n");

    fprintf(stderr, "<start_point_x>         Draw the starting point x axis position of the content. Note max:65535\n");
    fprintf(stderr, "<start_point_y>         Draw the starting point y axis position of the content. Note max:65535\n");

    fprintf(stderr, "<box_width>             Draws the width of the content. Note max:65535\n");
    fprintf(stderr, "<box_height>            Draws the height of the content. Note max:65535\n");

    fprintf(stderr, "<line_width>            Draw the line width of content.\n");
    fprintf(stderr, "<line_lenght>           Draw the line width of content. It is only used in DBOX_MODE_RANGE\n");

    fprintf(stderr, "<color>                 The drawing content colors are red, green, black and yellow.\n");

    fprintf(stderr, "\ndraw rectangle\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s<mode=draw_rectangle> <background_image=background_image_path> <is_rgba=0> \n" \
                    "    <background_width=value> <background_height=value> <output_file=output_file> \n" \
                    "    <start_point_x=value> <start_point_y=value> <box_width=value> <box_height=value> \n" \
                    "    <line_width=value> <line_lenght=value> <color=value>\n", command);
    fprintf(stderr, "Example:\n" \
                    "    %s mode=draw_rectangle background_image=/tmp/test.nv12 is_rgba=0 \n"\
                    "    background_width=640 background_height=480 output_file=/tmp/box.nv12 \n" \
                    "    start_point_x=100 start_point_y=100 box_width=440 box_height=280 line_width=4 \n" \
                    "    line_lenght=0 color=red\n", command);

    fprintf(stderr, "\ndraw horizontal line\n");
    fprintf(stderr, "Usage:\n" \
                    "    %s <mode=draw_horizontal_line> <background_image=background_pic_path> <is_rgba=value> \n" \
                    "    <background_width=value> <background_height=value> <output_file=output_file> \n" \
                    "    <start_point_x=value> <start_point_y=value> <box_width=value> \n" \
                    "    <line_width=value> <line_lenght=value> <color=value>\n", command);
    fprintf(stderr, "Example:\n " \
                    "    %s mode=draw_horizontal_line background_image=/tmp/test.nv12 is_rgba=0 \n" \
                    "    background_width=640 background_height=480 output_file=/tmp/hor.nv12 \n"\
                    "    start_point_x=0 start_point_y=238 box_width=640 box_height=0 \n" \
                    "    line_width=4 line_lenght=0 color=green\n", command);

    fprintf(stderr, "\ndraw vertical line\n");
    fprintf(stderr, "Usage:\n" \
                    "    %s <mode=draw_vertical_line> <background_image=background_pic_path> <is_rgba=value> \n" \
                    "    <background_width=value> <background_height=value> <output_file=output_file> \n" \
                    "    <start_point_x=value> <start_point_y=value> <box_height=value> \n" \
                    "    <line_width=value> <line_lenght=value> <color=value>\n", command);
    fprintf(stderr, "Example:\n" \
                    "    %s mode=draw_vertical_line background_image=/tmp/test.nv12 is_rgba=0 \n" \
                    "    background_width=640 background_height=480 output_file=/tmp/hor.nv12 \n" \
                    "    start_point_x=318 start_point_y=0 box_width=0 box_height=480 \n" \
                    "    line_width=4 line_lenght=0 color=black\n", command);
}

static int read_file_to_mem(const char *path, void *addr, unsigned int size)
{
    unsigned int ret = 0;

    int fd = open(path, O_RDONLY, 0777);
    if (fd < 0) {
        fprintf(stderr, "Error read open %s fail \n", path);
        return -1;
    }

    ret = read(fd, addr, size);
    close(fd);

    MAY_UNUSED(ret);

    return 0;
}

static int save_mem_to_file(const char *path, void *addr, unsigned int size)
{
    unsigned int ret = 0;

    FILE * pic_fp = fopen(path, "wb");
    if (pic_fp < 0) {
        fprintf(stderr, "Error save open %s fail \n", path);
        return -1;
    }

    ret = fwrite((void *)addr, 1, size, pic_fp);
    fclose(pic_fp);
    fflush(stdout);

    MAY_UNUSED(ret);

    return 0;
}

static void parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len)){
        return ;
    }

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        exit(-1);
    }

    *value = v;
}

static void parse_string(const char *str, const char *prefix, char **pos)
{
    int len_pre = strlen(prefix);
    int len_src = strlen(str);
    len_src = len_src - len_pre;

    if (strncmp(str, prefix, len_pre)) {
        return ;
    }

    if ( len_src > 0) {
        *pos = ((char *)str+len_pre);
    }
}

static void parse_string_mode(const char *str, const parse_info_t *info,
                              const char *string_mode, int *value)
{
    int pre_len = strlen(string_mode);
    int i = 0;

    if (strncmp(str, string_mode, pre_len)){
        return ;
    }

    for (i = 0; info[i].index != INDEX_END; i++) {
        if (strncmp((char *)str + pre_len, info[i].name, strlen(str) - pre_len ) == 0){
            *value = info[i].index;
            return ;
        }
    }
}

static void dump_dbox_info(const dbox_param_t *dbox_param)
{
    int i = 0;
    fprintf(stderr, "==================start dump parse dbox info==================\n");
    fprintf(stderr, "box_num = %d\n", dbox_param->boxs_num);
    fprintf(stderr, "dbox_param->img_w = %d\n", dbox_param->img_w);
    fprintf(stderr, "dbox_param->img_h = %d\n", dbox_param->img_h);
    fprintf(stderr, "dbox_param->is_rgba = %d\n", dbox_param->is_rgba);
    for(i = 0; i < dbox_param->boxs_num; i++) {
        fprintf(stderr, "dbox_param->ram_para[%d].box_mode = %d\n",i, dbox_param->ram_para[i].box_mode);
        fprintf(stderr, "dbox_param->ram_para[%d].color_mode = %d\n", i, dbox_param->ram_para[i].color_mode);
        fprintf(stderr, "dbox_param->ram_para[%d].start_point_x = %d\n", i, dbox_param->ram_para[i].start_point_x);
        fprintf(stderr, "dbox_param->ram_para[%d].start_point_y = %d\n", i, dbox_param->ram_para[i].start_point_y);
        fprintf(stderr, "dbox_param->ram_para[%d].box_width = %d\n", i, dbox_param->ram_para[i].box_width);
        fprintf(stderr, "dbox_param->ram_para[%d].box_height = %d\n", i, dbox_param->ram_para[i].box_height);
        fprintf(stderr, "dbox_param->ram_para[%d].line_width = %d\n", i, dbox_param->ram_para[i].line_width);
        fprintf(stderr, "dbox_param->ram_para[%d].line_height = %d\n", i, dbox_param->ram_para[i].line_lenght);
    }
    fprintf(stderr, "==================stop dump parse dbox info==================\n");
}

static void parse_cmd_info(const int argc, const char **argv, dbox_param_t *dbox_param, char **input_path, char **output_path)
{
    int bg_w = -1;
    int bg_h = -1;
    int is_rgba = -1;
    int start_point_x = -1;
    int start_point_y = -1;
    int box_width = -1;
    int box_height = -1;
    int line_width = -1;
    int line_lenght = -1;
    int color = -1;
    int mode = -1;
    int i = 0;

    for ( i = 1; i < argc; i++) {
        parse_string((const char *)argv[i], (const char *)"background_image=", (input_path));
        parse_string((const char *)argv[i], (const char *)"output_file=", (output_path));

        parse_string_mode((const char *)argv[i], (const parse_info_t *)&g_mode_info, "mode=", &mode);
        parse_string_mode((const char *)argv[i], (const parse_info_t *)&g_color_info, "color=", &color);

        parse_uint((const char *)argv[i], "is_rgba=", &is_rgba, 10);
        parse_uint((const char *)argv[i], "background_width=", &bg_w, 10);
        parse_uint((const char *)argv[i], "background_height=", &bg_h, 10);
        parse_uint((const char *)argv[i], "start_point_x=", &start_point_x, 10);
        parse_uint((const char *)argv[i], "start_point_y=", &start_point_y, 10);
        parse_uint((const char *)argv[i], "box_width=", &box_width, 10);
        parse_uint((const char *)argv[i], "box_height=", &box_height, 10);
        parse_uint((const char *)argv[i], "line_width=", &line_width, 10);
        parse_uint((const char *)argv[i], "line_lenght=", &line_lenght, 10);
    }

    if (mode < 0) {
        fprintf(stderr, "mode not support!\n");
        exit(-1);
    }
    if ( (is_rgba < 0) || (is_rgba > 1)) {
        fprintf(stderr, "is_rgba info err!\n");
        exit(-1);
    }
    if ( bg_w <= 0 || bg_h <= 0 ) {
        fprintf(stderr, "background_image size info err!\n");
        exit(-1);
    }
    if ( start_point_x < 0 || start_point_y < 0) {
        fprintf(stderr, "start_point info err!\n");
        exit(-1);
    }
    if ( box_width < 0 || box_height < 0 ) {
        fprintf(stderr, "draw box info format err!\n");
        exit(-1);
    }
    if (line_width < 0 || line_lenght < 0) {
        fprintf(stderr, "draw line info err!\n");
        exit(-1);
    }
    if ( (color < DBOX_COLOR_RED) || (color > DBOX_COLOR_YELLOW)) {
        fprintf(stderr, "color format err!\n");
        exit(-1);
    }

    /* to make it easiter to check, draw one at a time */
    dbox_param->boxs_num = 1;
    dbox_param->img_w = ALIGN_DOWN(bg_w, 8);
    dbox_param->img_h = ALIGN_DOWN(bg_h, 8);
    dbox_param->is_rgba = is_rgba;
    dbox_param->ram_para[0].box_mode = mode;
    dbox_param->ram_para[0].color_mode = color;
    dbox_param->ram_para[0].start_point_x = start_point_x;
    dbox_param->ram_para[0].start_point_y = start_point_y;
    dbox_param->ram_para[0].box_width = box_width;
    dbox_param->ram_para[0].box_height = box_height;
    dbox_param->ram_para[0].line_width = line_width;
    dbox_param->ram_para[0].line_lenght = line_lenght;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int dbox_fd = 0;
    char *input_path = NULL;
    char *output_path = NULL;
    dbox_param_t dbox_param;
    memset((void *)&dbox_param, 0, sizeof(dbox_param_t));
    rmem_info_t rmem_info;
    memset((void *)&rmem_info, 0, sizeof(rmem_info_t));

    int size = 0;

    if (argc != 14) {
        cmd_usage(argv[0]);
        exit(-1);
    }

    parse_cmd_info((const int)argc, (const char **)argv, &dbox_param, &input_path, &output_path);

    dbox_fd = dbox_open();
    if (dbox_fd < 0) {
        fprintf(stderr, "dbox_open fail!\n");
        goto exit_err;
    }

    if (dbox_param.is_rgba)
        size = dbox_param.img_w*dbox_param.img_h*4;
    else
        size = dbox_param.img_w*dbox_param.img_h*3/2;

    /* It is used to apply for a physical continuous address */
    rmem_info.fd = rmem_open();
    if (rmem_info.fd < 0) {
        fprintf(stderr, "rmem_open fail!\n");
        goto rmem_open_err;
    }
    rmem_info.mmap_addr = rmem_alloc(rmem_info.fd, &(rmem_info.phy_addr), size);
    if (rmem_info.mmap_addr == NULL) {
        fprintf(stderr, "%s : alloc rmem space for background picture fail\n", __func__);
        goto rmem_alloc_err;
    }
    rmem_info.size = size;

    memset(rmem_info.mmap_addr, 0, rmem_info.size);
    read_file_to_mem((const char *)input_path, rmem_info.mmap_addr, size);
    rmem_cache_sync(rmem_info.fd, rmem_info.mmap_addr, rmem_info.size, rmem_cache_mem_to_dev);

    dbox_param.box_pbuf = rmem_info.phy_addr;
    ret = dbox_start_draw((const int)dbox_fd, (const dbox_param_t *)&dbox_param);
    if (ret) {
        fprintf(stderr, "dbox_start_draw fail!\n");
        dump_dbox_info((const dbox_param_t *)&dbox_param);
        goto dbox_start_err;
    }

    rmem_cache_sync(rmem_info.fd, rmem_info.mmap_addr, rmem_info.size, rmem_cache_dev_to_mem);
    save_mem_to_file((const char *)output_path, rmem_info.mmap_addr, rmem_info.size);

dbox_start_err:
    rmem_free(rmem_info.fd, rmem_info.mmap_addr, rmem_info.phy_addr, rmem_info.size);
rmem_alloc_err:
    rmem_close(rmem_info.fd);
rmem_open_err:
    dbox_close(dbox_fd);
exit_err:
    return ret;
}