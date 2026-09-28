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

static const parse_info_t g_piexl_format[] = {
    { HAL_PIXEL_FORMAT_NV21,            "NV21"},
    { HAL_PIXEL_FORMAT_BGRA_8888,       "BGRA_8888"},
    { HAL_PIXEL_FORMAT_HSV,             "HSV"},
    INFO_END
};

static void cmd_usage(const char *command)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s <input_image=image_path> <output_image=image_path>\n"
                    "<output_fmt=fmt> <image_width=value> <image_height=value>\n", command);

    fprintf(stderr, "\nArguments as follow\n"
                    "<input_image>      The background image will be used to color conversion.\n"
                    "<output_image>     The output image after color conversion.\n"
                    "<output_fmt>       The output file formats are NV21, BGRA_8888 and HSV.\n"
                    "<image_width>      The background image weight.\n"
                    "<image_height>     The background image height.\n");

    fprintf(stderr, "Example:\n"
                    "    %s input_image=/tmp/test.nv12 output_image=/tmp/test.argb\n"
                    "output_fmt=BGRA_8888 image_width=640 image_height=480\n", command);

    fprintf(stderr, "Note:\n"
                    "    Minimum input image size (pixel): 4x4.\n"
                    "    Maximum input/output image size (pixel): 2592x2048.\n"
                    "    Input background channel picture format must be NV12.\n"
                    "    The memory of the images involved in the operation must be physically contiguous.\n");

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

    if (fmt_is_yuv(fmt)) {
        size = w*h*3/2;
    }
    else {
        /* according to the manual, HSV(H:2Byte S:1Byte V:1Byte) */
        size = w*h*4;
    }
    return size;
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

static void parse_cmd_info(const int argc, const char **argv, ipu_param_t *ipu_param,
                           char **input_path, char **output_path)
{
    int image_width = 0;
    int image_height = 0;
    int output_fmt = -1;
    int i = 0;

    for (i = 1; i < argc; i++) {
        parse_string((const char *)argv[i], "input_image=", input_path);
        parse_string((const char *)argv[i], "output_image=", output_path);

        parse_format((const char *)argv[i], "output_fmt=", &output_fmt);

        parse_uint((const char *)argv[i], "image_width=", &image_width, 10);
        parse_uint((const char *)argv[i], "image_height=", &image_height, 10);
    }

    if ( image_width <= 0 || image_height <= 0 ) {
        fprintf(stderr, "input_image size err!\n");
        exit(-1);
    }
    if ( output_fmt <= INDEX_END) {
        fprintf(stderr, "output_fmt err!\n");
        exit(-1);
    }

    ipu_param->cmd |= IPU_CMD_CSC_FLAG;
    ipu_param->bg_w = ALIGN_DOWN(image_width, 8);
    ipu_param->bg_h = ALIGN_DOWN(image_height, 8);

    /* input background picture format must be nv12*/
    ipu_param->bg_fmt = HAL_PIXEL_FORMAT_NV12;
    ipu_param->out_fmt = output_fmt;
}

static int ipu_param_fill_buf_addr(ipu_param_t *ipu_param, unsigned long src_buf_addr,
                                   unsigned long dst_buf_addr)
{
    int ret = -1;

    if ( src_buf_addr == 0 || dst_buf_addr == 0) {
        fprintf(stderr, "ipu osd phy can't be null!\n");
        return ret;
    }

    ipu_param->bg_buf_phy = src_buf_addr;

    /* fixed ipu_osdx_param[0].osd_chx_buf_phy as output phy addr in ipu driver */
    ipu_param->ipu_osdx_param[0].osd_chx_buf_phy = dst_buf_addr;

    return ret;
}

static void dump_ipu_info(const ipu_param_t *ipu_param)
{
    if (ipu_param == NULL) {
        fprintf(stderr, "ipu_param is NULL!\n");
    }

    fprintf(stderr, "====start dump ipu info ====\n");
    fprintf(stderr, "cmd=%d\n", ipu_param->cmd);
    fprintf(stderr, "bg_w=%d\n", ipu_param->bg_w);
    fprintf(stderr, "bg_h=%d\n", ipu_param->bg_h);
    fprintf(stderr, "bg_fmt=%d\n", ipu_param->bg_fmt);
    fprintf(stderr, "out_fmt=%d\n", ipu_param->out_fmt);
    fprintf(stderr, "bg_phy_addr=0x%x\n", ipu_param->bg_buf_phy);
    fprintf(stderr, "output_phy_addr=0x%x\n", ipu_param->ipu_osdx_param[0].osd_chx_buf_phy);
    fprintf(stderr, "====stop dump ipu info ====\n");

}
int main(int argc, char *argv[])
{
    int ret = 0;
    int ipu_fd = 0;
    int rmem_fd = 0;
    char *input_path = NULL;
    char *output_path = NULL;
    ipu_param_t ipu_param;
    memset((void *)&ipu_param, 0, sizeof(ipu_param_t));
    image_info_t input_image_info;
    memset((void *)&input_image_info, 0, sizeof(image_info_t));
    image_info_t output_image_info;
    memset((void *)&output_image_info, 0, sizeof(image_info_t));

    if (argc != 6){
        cmd_usage((const char *)argv[0]);
        goto exit_err;
    }

    parse_cmd_info((const int)argc, (const char **)argv, &ipu_param, &input_path, &output_path);

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

    /* according to the manual, input background picture format must be nv12 */
    input_image_info.size = get_image_size(ipu_param.bg_fmt, ipu_param.bg_h, ipu_param.bg_w);
    input_image_info.mmap_addr = rmem_alloc(rmem_fd, &(input_image_info.phy_addr), input_image_info.size);
    if (input_image_info.mmap_addr == NULL) {
        fprintf(stderr, "%s : alloc rmem space for background picture fail\n", __func__);
        goto rmem_input_alloc_err;
    }
    memset(input_image_info.mmap_addr, 0, input_image_info.size);
    read_file_to_mem((const char *)input_path, input_image_info.mmap_addr, input_image_info.size);
    rmem_cache_sync(rmem_fd, input_image_info.mmap_addr, input_image_info.size, rmem_cache_mem_to_dev);

    /* rmem for output_image */
    output_image_info.size = get_image_size(ipu_param.out_fmt, ipu_param.bg_h ,ipu_param.bg_w);
    output_image_info.mmap_addr = rmem_alloc(rmem_fd, &(output_image_info.phy_addr), output_image_info.size);
    if (output_image_info.mmap_addr == NULL) {
        fprintf(stderr, "%s : alloc rmem space for output_image fail\n", __func__);
        goto rmem_output_alloc_err;
    }
    memset(output_image_info.mmap_addr, 0, output_image_info.size);

    ipu_param_fill_buf_addr(&ipu_param, input_image_info.phy_addr, output_image_info.phy_addr);
    ret = ipu_start_draw((const int)ipu_fd, (const ipu_param_t *)&ipu_param);
    if (ret) {
        fprintf(stderr, "ipu_start_draw fail!\n");
        dump_ipu_info((const ipu_param_t *)&ipu_param);
        goto ipu_start_err;
    }

    rmem_cache_sync(rmem_fd, output_image_info.mmap_addr, output_image_info.size, rmem_cache_dev_to_mem);
    save_mem_to_file((const char *)output_path, output_image_info.mmap_addr, output_image_info.size);

ipu_start_err:
    rmem_free(rmem_fd, output_image_info.mmap_addr, output_image_info.phy_addr, output_image_info.size);
rmem_output_alloc_err:
    rmem_free(rmem_fd, input_image_info.mmap_addr, input_image_info.phy_addr, input_image_info.size);
rmem_input_alloc_err:
    rmem_close(rmem_fd);
rmem_open_err:
    ipu_close(ipu_fd);
exit_err:
    return ret;
}

