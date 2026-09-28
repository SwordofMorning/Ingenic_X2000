/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <malloc.h>
#include <isp.h>
#include <isp_tuning_x2580.h>

#define TEST_ISP_TUNING_TOP_CONTROL          0  //TOP CONTROL
#define TEST_ISP_TUNING_HVFLIP               0  //水平/垂直　翻转
#define TEST_ISP_TUNING_RUNNING_MODE         0  //运行模式（day or night）
#define TEST_ISP_TUNING_CSC_ATTR             0  //CSC_ATTR
#define TEST_ISP_TUNING_CCM_ATTR             0  //CSC_ATTR
#define TEST_ISP_TUNING_GAMMA                0  //gamma
#define TEST_ISP_TUNING_FLICKER              0  //抗频闪
#define TEST_ISP_TUNING_AUTOZOOM_CONTROL     0  //AUTOZOOM_CONTROL
#define TEST_ISP_TUNING_SENSOR_FPS           0  //设置帧率
#define TEST_ISP_TUNING_SENSOR_ATTR          0  //SENSOR_ATTR
#define TEST_ISP_TUNING_AWB_WEIGHT           0  //AWB_WEIGHT
#define TEST_ISP_TUNING_AWB_ATTR             0  //AWB_ATTR
#define TEST_ISP_TUNING_AWB_STATIS           0  //AWB_STATIS
#define TEST_ISP_TUNING_AWB_GLOBAL_STATIS    0  //AWB_GLOBAL_STATIS
#define TEST_ISP_TUNING_AE_SCENCE_ATTR       0  //AE_SCENCE_ATTR
#define TEST_ISP_TUNING_AE_EXPR_INFO         0  //AE_EXPR_INFO
#define TEST_ISP_TUNING_AE_STATIS            0  //AE_STATIS
#define TEST_ISP_TUNING_AE_WEIGHT            0  //AE_WEIGHT
#define TEST_ISP_TUNING_AE_EXP_LIST          0  //AE_EXP_LIST
#define TEST_ISP_TUNINGS_STATIS_CONFIG       0  //STATIS_CONFIG

static int dev_fd;
static int fd_tuning;

static struct camera_info info;

static struct frame_image_format output_fmt = {
    .width              = 1920,
    .height             = 1080,
    .pixel_format       = CAMERA_PIX_FMT_NV12,

    .scaler.enable      = 0,
    .scaler.width       = 1920,
    .scaler.height      = 1200,

    .crop.enable        = 0,
    .crop.top           = 0,
    .crop.left          = 0,
    .crop.width         = 1920,
    .crop.height        = 1080,

    .frame_nums         = 2,
};

static void usage(const char *prg_name, int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                        : show help info\n");
    fprintf(stderr, "    device_path                      : start frame\n");
    fprintf(stderr, "    Example: %s  /dev/mscaler-ch0\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

static inline uint64_t boot_time_usecs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 * 1000 + ts.tv_nsec / 1000;
}

static int save_frame_to_file(void *mem, int size)
{
    int fd;
    char *file_name = "/tmp/frame.nv12";
    int ret = 0;

    fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0644);
    if (fd < 0) {
        fprintf(stderr, "open file[%s] failed\n", file_name);
        return -1;
    }

    lseek(fd, 0, SEEK_SET);
    ret = write(fd, mem, size);
    if (ret < 0) {
        fprintf(stderr, "write to file(%s) failed ret=%d\n", file_name, ret);
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);

    system("sync");
    fprintf(stderr, "Save Frame to File(%s) size=%d\n", file_name, size);

    return 0;
}


static void signal_handler(int signum)
{
    int fd = dev_fd;
    isp_stream_off(fd);

    isp_power_off(fd);

    isp_free_buffer(fd);

    isp_close(fd, &info);

    dev_fd = -1;
    exit(-1);
}

int main(int argc, char *argv[])
{
    int ret = 0;

    /* 参数检查 */
    if (argc < 2)
        usage(argv[0], -1);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argv[0], argc == 2 ? 0 : -1);

    signal(SIGINT, signal_handler);

    malloc_trim(0); //free idle Memory

    int fd = isp_open(argv[1]);
    if (fd == -ENODEV)
        return -ENODEV;

    dev_fd = fd;

    if (!output_fmt.scaler.enable && !output_fmt.crop.enable) {
        if (isp_get_sensor_info(fd, &info) == 0) {
            output_fmt.width = info.width;
            output_fmt.height = info.height;
        }
    }

    ret = isp_set_format(fd, &output_fmt);
    if (ret < 0) {
        fprintf(stderr, "isp set format failed\n");
        close(fd);
        return ret;
    }

    ret = isp_requset_buffer(fd, &output_fmt);
    if (ret < 0) {
        fprintf(stderr, "isp set request buffer failed\n");
        close(fd);
        return ret;
    }

    isp_get_info(fd, &info);

    ret = isp_mmap(fd, &info);
    if (ret < 0) {
        isp_free_buffer(fd);
        close(fd);
        return ret;
    }

    ret = isp_power_on(fd);
    if (ret)
        goto close_fd;

    ret = isp_stream_on(fd);
    if (ret)
        goto close_fd;

    char fmt_a = (char)(info.data_fmt >> 0);
    char fmt_b = (char)(info.data_fmt >> 8);
    char fmt_c = (char)(info.data_fmt >> 16);
    char fmt_d = (char)(info.data_fmt >> 24);
    fprintf(stderr, "name               : %s\n", info.name);
    fprintf(stderr, "width              : %d\n", info.width);
    fprintf(stderr, "height             : %d\n", info.height);
    fprintf(stderr, "fps                : %d\n", info.fps);
    fprintf(stderr, "data_fmt           : %c%c%c%c\n", fmt_a, fmt_b, fmt_c, fmt_d);
    fprintf(stderr, "line_length        : %d\n", info.line_length);
    fprintf(stderr, "frame_size         : %d\n", info.frame_size);
    fprintf(stderr, "frame_nums         : %d\n", info.frame_nums);
    fprintf(stderr, "phys_mem           : %08lx\n", info.phys_mem);
    fprintf(stderr, "mapped_mem         : %p\n", info.mapped_mem);
    fprintf(stderr, "frame_align_size   : %d\n", info.frame_align_size);

    const char *tuning_dev = "/dev/isp-tuning";
    fd_tuning = isp_tuning_open(tuning_dev);
    fprintf(stderr, "\nOpen isp tuning device: %s\n", tuning_dev);
    if (fd_tuning == -ENODEV)
        return -ENODEV;

     sleep(1);

#if TEST_ISP_TUNING_TOP_CONTROL
    ISPModuleCtl isp_module;
    isp_tuning_get_module_control(fd_tuning, &isp_module);
    fprintf(stderr, "\nmodule_control:\n\n");
    fprintf(stderr, "\tisp_tuning_get_module_control, isp_module  =  0x%x\n", isp_module.key);
    isp_module.key = isp_module.key | 1 << 22;
    isp_tuning_set_module_control(fd_tuning, &isp_module);
    fprintf(stderr, "\tisp_tuning_get_module_control, isp_module  =  0x%x\n", isp_module.key);
    fprintf(stderr, "\n");

#endif

/* 保证ISP出图 */
#if TEST_ISP_TUNING_RUNNING_MODE
    isp_running_mode runing_mode_s, runing_mode_g;
    runing_mode_s = TISP_RUNING_MODE_DAY_MODE;
    fprintf(stderr, "\nrunning_mode:\n\t");
    isp_tuning_set_isp_running_mode(fd_tuning, &runing_mode_s);
    fprintf(stderr, "\tSet_isp_running_mode, runing_mode  =  %d\n", runing_mode_s);
    sleep(1);
    isp_tuning_get_isp_running_mode(fd_tuning, &runing_mode_g);
    fprintf(stderr, "\tGet_isp_running_mode, runing_mode  =  %d\n\n", runing_mode_g);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_HVFLIP
    tisp_hv_flip_t ops_mode_hvflip_g;
    isp_tuning_get_isp_hvflip(fd_tuning, &ops_mode_hvflip_g);
    fprintf(stderr, "\nisp_hvflip:\n");
    fprintf(stderr, "\tGet_isp_hvflip, ops_mode_hvflip  =  %d\n\n", ops_mode_hvflip_g.isp_mode[0]);
    sleep(1);   //下帧生效
    ops_mode_hvflip_g.isp_mode[0] = ISP_FLIP_ISP_HV_MODE;
    isp_tuning_set_isp_hvflip(fd_tuning, &ops_mode_hvflip_g);
    fprintf(stderr, "\tSet_isp_hvflip, ops_mode_hvflip  =  %d\n", ops_mode_hvflip_g.isp_mode[0]);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_CSC_ATTR
    isp_csc_attr csc_attr;
    csc_attr.ColorGamut = ISP_CG_BT709_FULL;
    isp_tuning_set_isp_csc_attr(fd_tuning, &csc_attr);
    isp_tuning_get_isp_csc_attr(fd_tuning, &csc_attr);
    fprintf(stderr, "\nisp_csc_attr:\n\t");
    fprintf(stderr, "isp_tuning_get_isp_csc_attr, ColorGamut  =  0x%x\n\n", csc_attr.ColorGamut);
    for(int i = 1; i <= 9; i++) {
        fprintf(stderr, "\t\t%.3f ", csc_attr.Matrix.CscCoef[i-1]);
        if(i % 3 == 0)
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

#endif

#if TEST_ISP_TUNING_CCM_ATTR
    isp_ccm_attr ccm_attr;
    ccm_attr.ManualEn = ISP_TUNING_OPS_MODE_DISABLE;
    ccm_attr.SatEn = ISP_TUNING_OPS_MODE_DISABLE;
    isp_tuning_set_isp_ccm_attr(fd_tuning, &ccm_attr);
    isp_tuning_get_isp_ccm_attr(fd_tuning, &ccm_attr);
    fprintf(stderr, "\nisp_ccm_attr:\n");
    fprintf(stderr, "\tisp_tuning_get_isp_ccm_attr,ManualEn %d SatEn %d\n\n", ccm_attr.ManualEn, ccm_attr.SatEn);
    for(int i = 1; i <= 9; i++) {
        fprintf(stderr, "\t\t%.3f ", ccm_attr.ColorMatrix[i-1]);
        if(i % 3 == 0)
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

#endif


#if TEST_ISP_TUNING_GAMMA
    isp_gamma gamma_g;
    isp_gamma gamma_s = {
        .type = ISP_GAMMA_CURVE_REC709,
        .gamma = {
               0, 144, 288, 412, 526, 625, 714, 796, 871, 941,1007,1070,1129,1186,1240,1293,
            1343,1392,1440,1486,1530,1574,1616,1657,1697,1737,1775,1813,1850,1886,1922,1957,
            1991,2025,2058,2091,2123,2155,2186,2217,2247,2277,2306,2336,2364,2393,2421,2449,
            2476,2503,2530,2557,2583,2609,2635,2660,2685,2710,2735,2760,2784,2808,2832,2855,
            2879,2902,2925,2948,2970,2993,3015,3037,3059,3081,3103,3124,3146,3167,3188,3209,
            3229,3250,3270,3291,3311,3331,3351,3371,3390,3410,3429,3449,3468,3487,3506,3525,
            3543,3562,3581,3599,3617,3636,3654,3672,3690,3708,3725,3743,3761,3778,3795,3813,
            3830,3847,3864,3881,3898,3915,3932,3948,3965,3981,3998,4014,4031,4047,4063,4079,
            4095
        }
    };
    isp_tuning_set_gamma(fd_tuning, &gamma_s);

    isp_tuning_get_gamma(fd_tuning, &gamma_g);
    fprintf(stderr, "\nGet_gamma[129]:\n\t");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 16; j++) {
            fprintf(stderr, "%04u ", gamma_g.gamma[i*16+j]);
        }
        fprintf(stderr, "\n\t");
    }
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_AUTOZOOM_CONTROL
    // to do
    isp_auto_zoom ispautozoom;
    ispautozoom.zoom_chx_en[0] = 1;
    ispautozoom.zoom_left[0] = 10;
    ispautozoom.zoom_top[0] = 10;
    ispautozoom.zoom_width[0] = 640;
    ispautozoom.zoom_height[0] = 480;
    isp_tuning_set_isp_auto_zoom(fd_tuning, &ispautozoom);
    sleep(1);
    isp_tuning_get_isp_auto_zoom(fd_tuning, &ispautozoom);
    fprintf(stderr, "\nisp_auto_zoom:\n");

    for(int i=0; i<3; i++){
        if (ispautozoom.zoom_chx_en[i]) {
            fprintf(stderr, "\tch:%d, left:%d, top:%d, width:%d, height:%d\n", i,
                ispautozoom.zoom_left[i], ispautozoom.zoom_top[i],
                ispautozoom.zoom_width[i], ispautozoom.zoom_height[i]);
        }
    }
    fprintf(stderr, "\n");

#endif

#if TEST_ISP_TUNING_FLICKER
    ISPAntiflickerAttr flikerattr;
    flikerattr.mode = ISP_ANTIFLICKER_AUTO_MODE;
    flikerattr.freq = 60;
    isp_tuning_set_anti_flicker_attr(fd_tuning, &flikerattr);
    sleep(1);
    isp_tuning_get_anti_flicker_attr(fd_tuning, &flikerattr);
    fprintf(stderr, "\nanti_flicker_attr:\n\t");
    fprintf(stderr, "Get_anti_flicker_attr, mode %d frequency = %d\n\n", flikerattr.mode, flikerattr.freq);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_SENSOR_FPS
    isp_sensor_fps fps;
    isp_tuning_get_sensor_fps(fd_tuning, &fps);
    fprintf(stderr, "\nsensor_fps:\n");
    fprintf(stderr, "\tisp_tuning_get_sensor_fps, fps = %.1f\n", (float)fps.num / fps.den);
    sleep(1);
    fps.num = 25;
    fps.den = 1;
    isp_tuning_set_sensor_fps(fd_tuning, &fps);
    fprintf(stderr, "\tisp_tuning_set_sensor_fps, fps = %.1f\n\n", (float)fps.num / fps.den);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_SENSOR_ATTR
    isp_sensor_attr sensor_attr;
    isp_tuning_get_sensor_attr(fd_tuning, &sensor_attr);
    fprintf(stderr, "\nsensor_attr:\n");
    fprintf(stderr, "\tfps %d, height %d, hts %d, vts %d, width %d\n", \
            sensor_attr.fps, sensor_attr.height, sensor_attr.hts, sensor_attr.vts, sensor_attr.width);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_AWB_WEIGHT
    isp_weight weight = {
        .weight = {
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        }
    };
    isp_tuning_set_awb_weight(fd_tuning, &weight);
    sleep(1);
    isp_tuning_get_awb_weight(fd_tuning, &weight);
    fprintf(stderr, "ae_weight[15][15]: \n\t");
    fprintf(stderr, "    00 01 02 03 04 05 06 07 08 09 10 11 12 13 14\n\t");
    for (int i = 0; i < 15; i++) {
        fprintf(stderr, "%02d:", i);
        for (int j = 0; j < 15; j++) {
            fprintf(stderr, "  %u", weight.weight[i][j]);
        }
        fprintf(stderr, "\n\t");
    }
    fprintf(stderr, "\n");

#endif

#if TEST_ISP_TUNING_AWB_ATTR
    isp_awb_attr awb_attr;
    awb_attr.mode = ISP_CORE_WB_MODE_MANUAL;
    awb_attr.gain_val.rgain = 200;
    awb_attr.gain_val.bgain = 200;
    awb_attr.awb_frz = ISP_TUNING_OPS_MODE_ENABLE;
    awb_attr.ct = 200;
    awb_attr.awb_start_en = ISP_TUNING_OPS_MODE_ENABLE;
    awb_attr.awb_start.rgain = 200;
    awb_attr.awb_start.bgain = 200;
    awb_attr.custom.customEn = ISP_TUNING_OPS_MODE_ENABLE;
    awb_attr.custom.gainH.rgain = 100;
    awb_attr.custom.gainH.bgain = 100;
    awb_attr.custom.gainM.rgain = 100;
    awb_attr.custom.gainM.bgain = 100;
    awb_attr.custom.gainL.rgain = 100;
    awb_attr.custom.gainL.bgain = 100;
    isp_tuning_set_awb_attr(fd_tuning, &awb_attr);
    sleep(1);
    isp_tuning_get_awb_attr(fd_tuning, &awb_attr);
    fprintf(stderr, "\nawb_attr:\n");
    fprintf(stderr, "\tawb mode:%d, rgain:%d, bgain:%d\n", awb_attr.mode, awb_attr.gain_val.rgain, awb_attr.gain_val.bgain);
    fprintf(stderr, "\tawb frzzen:%d, ct:%d\n", awb_attr.awb_frz, awb_attr.ct);
    fprintf(stderr, "\tawb start en:%d, start rgain:%d, start bgain:%d\n", awb_attr.awb_start_en, awb_attr.awb_start.rgain, awb_attr.awb_start.bgain);
    fprintf(stderr, "\tawb custom en:%d\n", awb_attr.custom.customEn);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_AWB_STATIS
    isp_awb_statis_info awb_statis_info;
    isp_tuning_get_awb_statis_zone(fd_tuning, &awb_statis_info);
    fprintf(stderr, "\nawb_statis_zone:\n");

    fprintf(stderr, "\n---- R ----\n");

    for (int i = 0; i < 15; i++) {
        fprintf(stderr, "\t%02d:", i);
        for (int j = 0; j < 15; j++) {
            fprintf(stderr, "  %u", awb_statis_info.awb_r.statis[i][j]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n---- G ----\n");
    for (int i = 0; i < 15; i++) {
        fprintf(stderr, "\t%02d:", i);
        for (int j = 0; j < 15; j++) {
            fprintf(stderr, "  %u", awb_statis_info.awb_g.statis[i][j]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n---- B ----\n");
    for (int i = 0; i < 15; i++) {
        fprintf(stderr, "\t%02d:", i);
        for (int j = 0; j < 15; j++) {
            fprintf(stderr, "  %u", awb_statis_info.awb_b.statis[i][j]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
#endif


#if TEST_ISP_TUNING_AWB_GLOBAL_STATIS
    isp_awb_statics_global awb_statics_global;
    isp_tuning_get_awb_global_statis(fd_tuning, &awb_statics_global);
    fprintf(stderr, "\nawb_statics_global:\n");
    fprintf(stderr, "\tstatis_gol_gain.rgain:%d\n", awb_statics_global.statis_gol_gain.rgain);
    fprintf(stderr, "\tstatis_gol_gain.bgain:%d\n", awb_statics_global.statis_gol_gain.bgain);
    fprintf(stderr, "\tstatis_weight_gain.rgain:%d\n", awb_statics_global.statis_weight_gain.rgain);
    fprintf(stderr, "\tstatis_weight_gain.bgain:%d\n", awb_statics_global.statis_weight_gain.bgain);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_AE_SCENCE_ATTR
    isp_ae_scence_attr ae_scence_attr;
    ae_scence_attr.AeHLCEn = 1;
    ae_scence_attr.AeHLCStrength = 1;
    isp_tuning_set_isp_ae_scence_attr(fd_tuning, &ae_scence_attr);
    sleep(1);
    isp_tuning_get_isp_ae_scence_attr(fd_tuning, &ae_scence_attr);
    fprintf(stderr, "\nae_scence_attr:\n");
    fprintf(stderr, "\tae hlcd mode:%d, strength:%d\n", ae_scence_attr.AeHLCEn, ae_scence_attr.AeHLCStrength);
    fprintf(stderr, "\tluma:%d, luma scence:%d\n", ae_scence_attr.luma, ae_scence_attr.luma_scence);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_AE_EXPR_INFO
    isp_ae_expr_info ae_expr_info;
    ae_expr_info.AeMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeIntegrationTimeMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeIntegrationTimeUnit = ISP_CORE_EXPR_UNIT_LINE;
    ae_expr_info.AeIntegrationTime = 100;
    ae_expr_info.AeAGainManualMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeAGain = 1024;
    ae_expr_info.AeMinIntegrationTimeMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeIntegrationTimeUnit = ISP_CORE_EXPR_UNIT_LINE;
    ae_expr_info.AeMinIntegrationTime = 4;
    ae_expr_info.AeMinAGainMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeMinAGain = 1024;

    ae_expr_info.AeShortMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeShortIntegrationTimeMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeIntegrationTimeUnit = ISP_CORE_EXPR_UNIT_LINE;
    ae_expr_info.AeShortIntegrationTime = 10;
    ae_expr_info.AeShortMinIntegrationTimeMode = ISP_TUNING_OPS_TYPE_MANUAL;
    ae_expr_info.AeIntegrationTimeUnit = ISP_CORE_EXPR_UNIT_LINE;
    ae_expr_info.AeShortMinIntegrationTime = 4;

    isp_tuning_set_isp_ae_expr_info(fd_tuning, &ae_expr_info);
    sleep(1);
    isp_tuning_get_isp_ae_expr_info(fd_tuning, &ae_expr_info);
    fprintf(stderr, "\nae_expr_info:\n");
    fprintf(stderr, "\tae integration time:%d\n", ae_expr_info.AeIntegrationTime);
    fprintf(stderr, "\tae min integration time:%d\n", ae_expr_info.AeMinIntegrationTime);
    fprintf(stderr, "\tae short integration time:%d\n", ae_expr_info.AeShortIntegrationTime);
    fprintf(stderr, "\tae short min integration time:%d\n", ae_expr_info.AeShortMinIntegrationTime);
    fprintf(stderr, "\ttotal gain:%d, short total gain:%d, exposure:%lld, ev:%d\n", ae_expr_info.TotalGainDb, \
                ae_expr_info.TotalGainDbShort, ae_expr_info.ExposureValue, ae_expr_info.EVLog2);
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_AE_STATIS
    isp_ae_statis_attr ae_statis_attr;
    isp_tuning_get_isp_ae_statis_attr(fd_tuning, &ae_statis_attr);
    fprintf(stderr, "\nae_statis_attr:\n");
    fprintf(stderr, "\nae hist 5bin:\n");
    for(int i = 0; i < 5; i++) {
        fprintf(stderr, "\t[%2d] : %4d\n", i, ae_statis_attr.ae_hist_5bin[i]);
    }
    fprintf(stderr, "\nae hist 256bin:\n");
    for(int i = 0; i < 256; i++) {
        fprintf(stderr, "\t[%d]:%d ", i, ae_statis_attr.ae_hist_256bin[i]);
        if(i % 15 == 0)
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\nae hist statis:\n");
    for (int i = 0; i < 15; i++) {
        fprintf(stderr, "\t%02d:", i);
        for (int j = 0; j < 15; j++) {
            fprintf(stderr, "  %u", ae_statis_attr.ae_statis.statis[i][j]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

#endif

#if TEST_ISP_TUNING_AE_WEIGHT
    isp_weight_attr weight_attr = {
        .weight_enable = ISP_TUNING_OPS_MODE_ENABLE,
        .ae_weight.weight = {
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 5, 5, 6, 5, 5, 5, 5, 0, 0, 0, 0},
            {0, 0, 0, 4, 5, 6, 7, 6, 6, 6, 5, 4, 0, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0},
            {0, 0, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 3, 0, 0}
        },
    };

    isp_tuning_set_isp_ae_weight(fd_tuning, &weight_attr);
    sleep(1);
    isp_tuning_get_isp_ae_weight(fd_tuning, &weight_attr);
    fprintf(stderr, "\nae_weight:\n");
    for (int i = 0; i < 15; i++) {
        fprintf(stderr, "\t%02d:", i);
        for (int j = 0; j < 15; j++) {
            fprintf(stderr, "  %u", weight_attr.ae_weight.weight[i][j]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

#endif

#if TEST_ISP_TUNINGS_STATIS_CONFIG
    isp_statis_config statis_config;
    unsigned char histThresh[4] = {0};
    statis_config.ae.ae_sta_en = ISP_TUNING_OPS_MODE_ENABLE;
    if (statis_config.ae.ae_sta_en == ISP_TUNING_OPS_MODE_ENABLE) {
        memcpy(statis_config.ae.histThresh, histThresh, sizeof(histThresh));
    }

    statis_config.awb.awb_sta_en = ISP_TUNING_OPS_MODE_ENABLE;
    if (statis_config.awb.awb_sta_en == ISP_TUNING_OPS_MODE_ENABLE) {
        statis_config.awb.local.start_h = 1;
        statis_config.awb.local.start_v = 1;
        statis_config.awb.local.node_h = 15;
        statis_config.awb.local.node_v = 15;
    }

    statis_config.af.af_sta_en = ISP_TUNING_OPS_MODE_ENABLE;
    if (statis_config.af.af_sta_en == ISP_TUNING_OPS_MODE_ENABLE) {
        statis_config.af.local.start_h = 1;
        statis_config.af.local.start_v = 3;
        statis_config.af.local.node_h = 15;
        statis_config.af.local.node_v = 15;
        statis_config.af.af_metrics_shift = 0;
        statis_config.af.af_delta = 1;
        statis_config.af.af_theta = 1;
        statis_config.af.af_hilight_th = 1;
        statis_config.af.af_alpha_alt = 1;
        statis_config.af.af_belta_alt = 1;
    }
    isp_tuning_set_isp_statis_config(fd_tuning, &statis_config);
    sleep(1);
    isp_tuning_get_isp_statis_config(fd_tuning, &statis_config);

    fprintf(stderr, "\nstatis_config:\n");
    fprintf(stderr, "\tae config mode:%s\n", statis_config.ae.ae_sta_en ? "enable" : "disable");
    if (statis_config.ae.ae_sta_en == ISP_TUNING_OPS_MODE_ENABLE) {
         fprintf(stderr, "\t\thistThresh[0-4] : %d %d %d %d \n", statis_config.ae.histThresh[0], statis_config.ae.histThresh[1], \
                    statis_config.ae.histThresh[2], statis_config.ae.histThresh[3]);
    }

    fprintf(stderr, "\tawb config mode:%s\n", statis_config.awb.awb_sta_en ? "enable" : "disable");
    if (statis_config.awb.awb_sta_en == ISP_TUNING_OPS_MODE_ENABLE) {
         fprintf(stderr, "\t\tstart_h %d start_v %d node_h %d node_v %d \n", statis_config.awb.local.start_h, statis_config.awb.local.start_v, \
                    statis_config.awb.local.node_h, statis_config.awb.local.node_v);
    }

    fprintf(stderr, "\taf config mode:%s\n", statis_config.af.af_sta_en ? "enable" : "disable");
    if (statis_config.af.af_sta_en == ISP_TUNING_OPS_MODE_ENABLE) {
         fprintf(stderr, "\t\tstart_h %d start_v %d node_h %d node_v %d \n", statis_config.af.local.start_h, statis_config.af.local.start_v,     \
                statis_config.af.local.node_h, statis_config.af.local.node_v);
    }
    fprintf(stderr, "\n");

#endif

#if TEST_ISP_TUNING_AE_EXP_LIST
    isp_ae_list_attr ae_list_attr;
    isp_tuning_set_isp_ae_list_attr(fd_tuning, &ae_list_attr);
    isp_tuning_get_isp_ae_list_attr(fd_tuning, &ae_list_attr);
#endif

    //等待图像稳定
    sleep(1);

    /* 获取frame_size(非对齐大小) */
    struct frame_image_format fmt;
    isp_get_format(fd, &fmt);

    struct frame_info frame;
    while (1) {
        ret = isp_dqbuf_wait(fd, &frame);
        if (0 == ret) {
            save_frame_to_file(frame.vaddr, frame.size);
            isp_qbuf(fd, &frame);
        }
        sleep(5);
    }

    isp_drop_frames(fd, info.frame_nums);

    isp_tuning_close(fd_tuning);
    isp_stream_off(fd);
    isp_power_off(fd);

close_fd:
    isp_free_buffer(fd);

    isp_close(fd, &info);
    dev_fd = -1;
    fd_tuning = -1;

    return ret;
}
