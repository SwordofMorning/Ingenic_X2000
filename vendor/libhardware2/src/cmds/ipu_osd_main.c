#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include <libhardware2/ipu.h>
#include <libhardware2/ipu_utils.h>
#include <libhardware2/rmem.h>

#define ALIGN_DOWN(n, align)            (n & (~(align - 1)))

#define MAY_UNUSED(n)                   ((n)=(n))

#define NAME_END                        (NULL)
#define INDEX_END                       (-1)
#define INFO_END                        {INDEX_END, NAME_END}

typedef struct {
    int size;
    void *mmap_addr;
    unsigned long phy_addr;
}image_info_t;

typedef struct {
    int index;
    const char *name;
}parse_info_t;

const static parse_info_t g_piexl_format[] = {
    { HAL_PIXEL_FORMAT_RGBA_8888,       "RGBA_8888"},
    { HAL_PIXEL_FORMAT_RGBX_8888,       "RGBX_8888"},
    { HAL_PIXEL_FORMAT_RGB_888,         "RGB_888"},
    { HAL_PIXEL_FORMAT_RGB_565,         "RGB_565"},
    { HAL_PIXEL_FORMAT_BGRA_8888,       "BGRA_8888"},

    { HAL_PIXEL_FORMAT_BGRX_8888,       "BGRX_8888"},
    { HAL_PIXEL_FORMAT_RGBA_5551,       "RGBA_5551"},
    { HAL_PIXEL_FORMAT_RGBA_4444,       "RGBA_4444"},
    { HAL_PIXEL_FORMAT_ABGR_8888,       "ABGR_8888"},
    { HAL_PIXEL_FORMAT_ARGB_8888,       "ARGB_8888"},
    { HAL_PIXEL_FORMAT_YCbCr_422_SP,    "YCbCr_422_SP"},
    { HAL_PIXEL_FORMAT_YCbCr_420_SP,    "YCbCr_420_SP"},
    { HAL_PIXEL_FORMAT_YCbCr_422_P,     "YCbCr_422_P"},
    { HAL_PIXEL_FORMAT_YCbCr_420_P,     "YCbCr_420_P"},
    { HAL_PIXEL_FORMAT_YCbCr_420_B,     "YCbCr_420_B"},
    { HAL_PIXEL_FORMAT_YCbCr_422_I,     "YCbCr_422_I"},
    { HAL_PIXEL_FORMAT_YCbCr_420_I,     "YCbCr_420_I"},
    { HAL_PIXEL_FORMAT_CbYCrY_422_I,    "CbYCrY_422_I"},
    { HAL_PIXEL_FORMAT_CbYCrY_420_I,    "CbYCrY_420_I"},
    { HAL_PIXEL_FORMAT_NV12,            "NV12"},
    { HAL_PIXEL_FORMAT_NV21,            "NV21"},
    { HAL_PIXEL_FORMAT_BGRA_5551,       "BGRA_5551"},
    { HAL_PIXEL_FORMAT_HSV,             "HSV"},

    { HAL_PIXEL_FORMAT_JZ_YUV_420_P,    "JZ_YUV_420_P"},
    { HAL_PIXEL_FORMAT_JZ_YUV_420_B,    "JZ_YUV_420_B"},
      INFO_END
};

static void cmd_usage(const char *command)
{
    int i = 0;
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s <src_image=image_path> <src_image_fmt=fmt>\n"
                     "<src_width=value> <src_height=value>\n"
                     "<dst_image=image_path> <dst_use_ch=value> <dst_image_fmt=fmt>\n"
                     "<dst_width=value> <dst_height=value>\n"
                     "<start_point_x=value> <start_point_y=value>\n"
                     "<output_fmt=fmt>  <output=image_path>\n", command);

    fprintf(stderr, "\nArguments as follow\n"
                    "<src_image>        The background image will be used for image overlay.\n"
                    "<src_image_fmt>    The src_image format.\n"
                    "<src_width>        The background image weight.\n"
                    "<src_height>       The background image height.\n"
                    "<dst_image>        The image will be superimposed over src_image.\n"
                    "<dst_use_ch>       The osd channels are 0, 1, 2, 3.\n"
                    "<dst_image_fmt>    The dst_image format.\n"
                    "<dst_width>        The overlay image width.\n"
                    "<dst_height>       The overlay image height.\n"
                    "<start_point_x>    The starting coordinates of dst_image in src_image.\n"
                    "<start_point_y>    The starting coordinates of dst_image in src_image.\n"
                    "<output_fmt>       The output file format.\n"
                    "<output_path>      The output file path after processing.\n\n");

    fprintf(stderr, "Example:\n"
                    "    %s src_image=/tmp/test.nv12 src_image_fmt=NV12 src_width=640 src_height=480\n"
                    "dst_image=/tmp/logo_bgra.rgb dst_use_ch=0 dst_image_fmt=BGRA_8888\n"
                    "dst_width=100 dst_height=100 start_point_x=0 start_point_y=0 output_fmt=NV12\n"
                    "output_path=/tmp/test_add_logo.nv12\n\n", command);

    fprintf(stderr, "Note:\n"
                    "    Minimum input image size (pixel): 4x4.\n"
                    "    Maximum input/output image size (pixel): 2592x2048.\n"
                    "    Input/Output background channel picture format must be NV12.\n"
                    "    Only argb_8888 pixel images can be superimposed onto nv12 pixel images currently.\n"
                    "    The memory of the images involved in the overlay operation must be physically contiguous.\n");

    fprintf(stderr, "fmt(Piexl format) as follows:\n");

    for ( i = 0; g_piexl_format[i].index != INDEX_END; i++) {
        fprintf(stderr, "    %s\n", g_piexl_format[i].name);
    }

}

static int read_file_to_mem(const char *path, void *addr, unsigned int size)
{
    unsigned int ret = 0;

    if (path == NULL) {
        fprintf(stderr, "Error read path is null!\n");
        return -1;
    }

    int fd = open(path, O_RDONLY, 0777);
    if (fd < 0) {
        fprintf(stderr, "Error read open %s fail \n", path);
        return -1;
    }

    ret = read(fd, addr, size);
    close(fd);

    MAY_UNUSED(ret);

    fprintf(stderr, "read file %s ret= 0x%x ok size= 0x%x !\n", path, ret, size);

    return 0;
}

static int save_mem_to_file(const char *path, void *addr, unsigned int size)
{
    unsigned int ret = 0;

    if (path == NULL) {
        fprintf(stderr, "Error save path is null!\n");
        return -1;
    }

    FILE * pic_fp = fopen(path, "wb");
    if (pic_fp < 0) {
        fprintf(stderr, "Error save open %s fail \n", path);
        return -1;
    }

    ret = fwrite((void *)addr, 1, size, pic_fp);
    fclose(pic_fp);
    fflush(stdout);

    fprintf(stderr, "save file %s ret= 0x%x ok size= 0x%x !\n", path, ret, size);
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

static void parse_format(const char *str, const char *str_mode, int *fmt_index)
{
    int i = 0;
    char *dst_fmt_str = NULL;
    parse_string(str, str_mode, &dst_fmt_str);

    if (dst_fmt_str == NULL) {
        return ;
    }

    for ( i = 0; g_piexl_format[i].index != INDEX_END; i++) {
        if (strncmp(dst_fmt_str, g_piexl_format[i].name, strlen(g_piexl_format[i].name)) == 0) {
            *fmt_index = g_piexl_format[i].index;
            return ;
        }
    }
}

static void dump_ipu_info(const ipu_param_t *ipu_param)
{
    int ch = 0;
    if (ipu_param == NULL) {
        fprintf(stderr, "ipu_param is NULL!\n");
    }
    fprintf(stderr, "====start dump ipu info ====\n");

    fprintf(stderr, "cmd=%d\n", ipu_param->cmd);
    fprintf(stderr, "bg_w=%d\n", ipu_param->bg_w);
    fprintf(stderr, "bg_h=%d\n", ipu_param->bg_h);
    fprintf(stderr, "bg_fmt=%d\n", ipu_param->bg_fmt);
    fprintf(stderr, "out_fmt=%d\n", ipu_param->out_fmt);
    fprintf(stderr, "bg_phy_addr=%d\n", ipu_param->bg_buf_phy);

    for (ch = IPU_CMD_OSD_CH0; ch <= IPU_CMD_OSD_CH3; ch++) {
        if (ipu_param->cmd & (1 << ch)) {
            fprintf(stderr, "osd_ch%d_fmt=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_fmt);
            fprintf(stderr, "osd_ch%d_para=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_para);
            fprintf(stderr, "osd_ch%d_fmt=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_bak_argb);
            fprintf(stderr, "osd_ch%d_src_w=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_src_w);
            fprintf(stderr, "osd_ch%d_src_h=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_src_h);
            fprintf(stderr, "osd_ch%d_pos_x=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_pos_x);
            fprintf(stderr, "osd_ch%d_pos_y=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_pos_y);
            fprintf(stderr, "osd_ch%d_buf_phy=%d\n", ch, ipu_param->ipu_osdx_param[ch].osd_chx_buf_phy);
        }
    }

    fprintf(stderr, "====stop dump ipu info ====\n");
}

static void parse_cmd_info(const int argc, const char **argv, ipu_param_t *ipu_param,
                           char **src_image, char **dst_image, char **output_path)
{
    int src_image_fmt = -1;
    int src_width = 0;
    int src_height = 0;
    int dst_image_fmt = -1;
    int dst_width = 0;
    int dst_height = 0;
    int start_point_x = 0;
    int start_point_y = 0;
    int output_fmt = -1;
    int ch = -1;
    int i = 0;

    for (i = 1; i < argc; i++) {
        parse_string((const char *)argv[i], "src_image=", src_image);
        parse_string((const char *)argv[i], "dst_image=", dst_image);
        parse_string((const char *)argv[i], "output_path=", output_path);

        parse_format((const char *)argv[i], "src_image_fmt=", &src_image_fmt);
        parse_format((const char *)argv[i], "dst_image_fmt=", &dst_image_fmt);
        parse_format((const char *)argv[i], "output_fmt=", &output_fmt);

        parse_uint((const char *)argv[i], "src_width=", &src_width, 10);
        parse_uint((const char *)argv[i], "src_height=", &src_height, 10);
        parse_uint((const char *)argv[i], "dst_use_ch=", &ch, 10);
        parse_uint((const char *)argv[i], "dst_width=", &dst_width, 10);
        parse_uint((const char *)argv[i], "dst_height=", &dst_height, 10);
        parse_uint((const char *)argv[i], "start_point_x=", &start_point_x, 10);
        parse_uint((const char *)argv[i], "start_point_y=", &start_point_y, 10);
    }

    if ( src_image_fmt <= INDEX_END) {
        fprintf(stderr, "src_image_fmt err!\n");
        exit(-1);
    }
    if ( src_width <= 0 || src_height <= 0 ) {
        fprintf(stderr, "src_image size err!\n");
        exit(-1);
    }
    if ( ch <= INDEX_END || ch > IPU_OSD_CH_INDEX_MAX ) {
        fprintf(stderr, "use channel err!\n");
        exit(-1);
    }
    if ( dst_image_fmt <= INDEX_END ) {
        fprintf(stderr, "dst_image_fmt err!\n");
        exit(-1);
    }
    if ( src_width <= 0 || src_height <= 0 ) {
        fprintf(stderr, "src_image size info err!\n");
        exit(-1);
    }
    if ( start_point_x < 0 || start_point_y < 0 ) {
        fprintf(stderr, "start_point info err!\n");
        exit(-1);
    }
    if ( output_fmt == INDEX_END) {
        fprintf(stderr, "output_fmt err!\n");
        exit(-1);
    }

    ipu_param->cmd |= (1 << ch);
    ipu_param->bg_w = ALIGN_DOWN(src_width, 8);
    ipu_param->bg_h = ALIGN_DOWN(src_height, 8);
    ipu_param->bg_fmt = src_image_fmt;
    ipu_param->out_fmt = output_fmt;

    ipu_param->ipu_osdx_param[ch].osd_chx_fmt = dst_image_fmt;

    /* the data sequence of AYUV or ARGB depend on CH0_MASK_EN */
    ipu_param->ipu_osdx_param[ch].osd_chx_bak_argb = 0x7f;
    ipu_param->ipu_osdx_param[ch].osd_chx_pos_x = start_point_x;
    ipu_param->ipu_osdx_param[ch].osd_chx_pos_y = start_point_y;
    ipu_param->ipu_osdx_param[ch].osd_chx_src_w = dst_width;
    ipu_param->ipu_osdx_param[ch].osd_chx_src_h = dst_height;
}

static int get_ipu_param_ch(const ipu_param_t *ipu_param)
{
    int ch = 0;
    for ( ch = IPU_CMD_OSD_CH0; ch <= IPU_CMD_OSD_CH3; ch++) {
        if (ipu_param->cmd & (1 << ch)) {
            return ch;
        }
    }
    return -1;
}

static int ipu_param_fill_buf_addr(ipu_param_t *ipu_param, unsigned long src_buf_addr, unsigned long dst_buf_addr)
{
    int ch = 0;
    int ret = -1;

    if ( src_buf_addr == 0 || dst_buf_addr == 0) {
        fprintf(stderr, "ipu osd phy can't be null!\n");
        return ret;
    }

    ipu_param->bg_buf_phy = src_buf_addr;

    for ( ch = IPU_CMD_OSD_CH0; ch <= IPU_CMD_OSD_CH3; ch++) {
        if (ipu_param->cmd & (1 << ch)) {
            ipu_param->ipu_osdx_param[ch].osd_chx_buf_phy = dst_buf_addr;
            ret = 0;
        }
    }

    return ret;
}

static unsigned int fmt_is_yuv(int bg_fmt)
{
    unsigned int is_yuv = 0;
    switch (bg_fmt) {
    case HAL_PIXEL_FORMAT_RGBA_5551:
    case HAL_PIXEL_FORMAT_BGRA_5551:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_ARGB_8888:
    case HAL_PIXEL_FORMAT_ABGR_8888:
    case HAL_PIXEL_FORMAT_RGBA_4444:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
        is_yuv = 0;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_422_P:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_JZ_YUV_420_P:
    case HAL_PIXEL_FORMAT_YCbCr_420_B:
    case HAL_PIXEL_FORMAT_JZ_YUV_420_B:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_YCbCr_420_I:
    case HAL_PIXEL_FORMAT_NV21:
    case HAL_PIXEL_FORMAT_NV12:
        is_yuv = 1;
        break;
    default:
        is_yuv = 0;
        break;
    }

    return is_yuv;
}

static int get_image_size(int fmt, int w, int h)
{
    int size = 0;

    if (fmt_is_yuv(fmt))
        size = w*h*3/2;
    else
        size = w*h*4;

    return size;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int ipu_fd = 0;
    int rmem_fd = 0;
    int ch = -1;
    char *src_image = NULL;
    char *dst_image = NULL;
    char *output_path = NULL;
    ipu_param_t ipu_param;
    memset((void *)&ipu_param, 0, sizeof(ipu_param_t));
    image_info_t src_image_info;
    memset((void *)&src_image_info, 0, sizeof(image_info_t));
    image_info_t dst_image_info;
    memset((void *)&dst_image_info, 0, sizeof(image_info_t));

    if (argc != 14){
        cmd_usage((const char *)argv[0]);
        goto exit_err;
    }

    parse_cmd_info((const int)argc, (const char **)argv, &ipu_param, &src_image, &dst_image, &output_path);

    ipu_fd = ipu_open();
    if (ipu_fd < 0) {
        fprintf(stderr, "ipu_open fail!\n");
        goto exit_err;
    }

    /* It is used to apply for a physical continuous mem space */
    rmem_fd = rmem_open();
    if (rmem_fd < 0) {
        fprintf(stderr, "rmem_open fail!\n");
        goto rmem_open_err;
    }

    /*rmem for src_image*/
    src_image_info.size = get_image_size(ipu_param.bg_fmt, ipu_param.bg_w, ipu_param.bg_h);
    src_image_info.mmap_addr = rmem_alloc(rmem_fd, &(src_image_info.phy_addr), src_image_info.size);
    if (src_image_info.mmap_addr == NULL) {
        fprintf(stderr, "%s : alloc rmem space for src_image fail\n", __func__);
        goto rmem_src_alloc_err;
    }

    memset(src_image_info.mmap_addr, 0, src_image_info.size);
    read_file_to_mem((const char *)src_image, src_image_info.mmap_addr, src_image_info.size);
    rmem_cache_sync(rmem_fd, src_image_info.mmap_addr, src_image_info.size, rmem_cache_mem_to_dev);

    /* rmem for dst_image */
    ch = get_ipu_param_ch((const ipu_param_t *)&ipu_param);
    dst_image_info.size = get_image_size(ipu_param.ipu_osdx_param[ch].osd_chx_fmt,
                                         ipu_param.ipu_osdx_param[ch].osd_chx_src_w,
                                         ipu_param.ipu_osdx_param[ch].osd_chx_src_h);
    dst_image_info.mmap_addr = rmem_alloc(rmem_fd, &(dst_image_info.phy_addr), dst_image_info.size);
    if (dst_image_info.mmap_addr == NULL) {
        fprintf(stderr, "%s : alloc rmem space for dst_image fail\n", __func__);
        goto rmem_dst_alloc_err;
    }

    memset(dst_image_info.mmap_addr, 0, dst_image_info.size);
    read_file_to_mem((const char *)dst_image, dst_image_info.mmap_addr, dst_image_info.size);
    rmem_cache_sync(rmem_fd, dst_image_info.mmap_addr, dst_image_info.size, rmem_cache_mem_to_dev);

    ipu_param_fill_buf_addr(&ipu_param, src_image_info.phy_addr, dst_image_info.phy_addr);
    ret = ipu_start_draw((const int)ipu_fd, (const ipu_param_t *)&ipu_param);
    if (ret) {
        fprintf(stderr, "ipu_start_draw fail!\n");
        dump_ipu_info((const ipu_param_t *)&ipu_param);
        goto ipu_start_err;
    }

    rmem_cache_sync(rmem_fd, dst_image_info.mmap_addr, dst_image_info.size, rmem_cache_dev_to_mem);
    save_mem_to_file((const char *)output_path, src_image_info.mmap_addr, src_image_info.size);

ipu_start_err:
    rmem_free(rmem_fd, dst_image_info.mmap_addr, dst_image_info.phy_addr, dst_image_info.size);
rmem_dst_alloc_err:
    rmem_free(rmem_fd, src_image_info.mmap_addr, src_image_info.phy_addr, src_image_info.size);
rmem_src_alloc_err:
    rmem_close(rmem_fd);
rmem_open_err:
    ipu_close(ipu_fd);
exit_err:
    return ret;
}