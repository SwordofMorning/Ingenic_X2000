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

#include <isp.h>
#include <isp_tuning.h>


#define TEST_ISP_TUNING_RUNNING_MODE         0  //运行模式（day or night）
#define TEST_ISP_TUNING_HFLIP                0  //水平翻转
#define TEST_ISP_TUNING_VFLIP                0  //垂直翻转
#define TEST_ISP_TUNING_SENSOR_FPS           0  //设置帧率
#define TEST_ISP_TUNING_BRIGHTNESS           0  //亮度
#define TEST_ISP_TUNING_CONTRAST             0  //对比度
#define TEST_ISP_TUNING_SATURATION           0  //饱和度
#define TEST_ISP_TUNING_SHARPNESS            0  //锐度
#define TEST_ISP_TUNING_ANTI_FLICKER_ATTR    0  //抗频闪
#define TEST_ISP_TUNING_EV_ATTR              0  //EV参数
#define TEST_ISP_TUNING_EXPR                 0  //曝光参数
#define TEST_ISP_TUNING_MAX_AGAIN            0  //最大模拟增益
#define TEST_ISP_TUNING_AE_MIN               0  //AE最小参数
#define TEST_ISP_TUNING_AE_LUMA              0  //AE亮度参数
#define TEST_ISP_TUNING_HI_LIGHT_DEPRESS     0  //强光抑制
#define TEST_ISP_TUNING_AE_WEIGHT            0  //AE区域权重
#define TEST_ISP_TUNING_WB                   0  //白平衡
#define TEST_ISP_TUNING_GAMMA                0  //gamma
#define TEST_ISP_TUNING_ADR_STRENGTH         0  //ADR强度

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
    fprintf(stderr, "    Example: %s  /dev/mscaler0-ch0\n", prg_name);
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

    const char *tuning_dev;
    if (!strncmp(argv[1], "/dev/mscaler0", 13)) {
        tuning_dev = "/dev/isp-tuning0";
        fd_tuning = isp_tuning_open(tuning_dev);
        fprintf(stderr, "\nOpen isp tuning device: %s\n", tuning_dev);

    } else if (!strncmp(argv[1], "/dev/mscaler1", 13)) {
        tuning_dev = "/dev/isp-tuning1";
        fd_tuning = isp_tuning_open(tuning_dev);
        fprintf(stderr, "\nOpen isp tuning device: %s\n", tuning_dev);
    }
    if (fd_tuning == -ENODEV)
        return -ENODEV;

     sleep(1);

/* 保证ISP出图 */
#if TEST_ISP_TUNING_RUNNING_MODE
    isp_running_mode runing_mode_s, runing_mode_g;
    runing_mode_s = ISP_RUNNING_MODE_DAY;
    isp_tuning_set_isp_running_mode(fd_tuning, runing_mode_s);
    fprintf(stderr, "----Set_isp_running_mode, runing_mode  =  %d\n", runing_mode_s);

    isp_tuning_get_isp_running_mode(fd_tuning, &runing_mode_g);
    fprintf(stderr, "----Get_isp_running_mode, runing_mode  =  %d\n\n", runing_mode_g);
#endif

#if TEST_ISP_TUNING_HFLIP
    isp_tuning_ops_mode ops_mode_hflip_g, ops_mode_hflip_s;
    ops_mode_hflip_s = ISP_TUNING_OPS_MODE_ENABLE;
    isp_tuning_set_isp_hflip(fd_tuning, ops_mode_hflip_s);
    fprintf(stderr, "----Set_isp_hflip, ops_mode_hflip  =  %d\n", ops_mode_hflip_s);
    sleep(1); //下帧生效
    isp_tuning_get_isp_hflip(fd_tuning, &ops_mode_hflip_g);
    fprintf(stderr, "----Get_isp_hflip, ops_mode_hflip  =  %d\n\n", ops_mode_hflip_g);
#endif

#if TEST_ISP_TUNING_VFLIP
    isp_tuning_ops_mode ops_mode_vflip_g, ops_mode_vflip_s;
    ops_mode_vflip_s = ISP_TUNING_OPS_MODE_ENABLE;
    isp_tuning_set_isp_vflip(fd_tuning, ops_mode_vflip_s);
    fprintf(stderr, "----Set_isp_hflip, ops_mode_vflip  =  %d\n", ops_mode_vflip_s);
    sleep(1);
    isp_tuning_get_isp_vflip(fd_tuning, &ops_mode_vflip_g);
    fprintf(stderr, "----Get_isp_hflip, ops_mode_vflip  =  %d\n\n", ops_mode_vflip_g);
#endif

#if TEST_ISP_TUNING_SENSOR_FPS
    uint32_t fps_num_g, fps_den_g, fps_num_s, fps_den_s;
    fps_num_s = 10;
    fps_den_s = 1;
    isp_tuning_set_sensor_fps(fd_tuning, fps_num_s, fps_den_s);
    fprintf(stderr, "----Set_sensor_fps, fps = %.1f\n", (float)fps_num_s/fps_den_s);

    isp_tuning_get_sensor_fps(fd_tuning, &fps_num_g, &fps_den_g);
    fprintf(stderr, "----Get_sensor_fps, fps = %.1f\n\n", (float)fps_num_g/fps_den_g);
#endif

#if TEST_ISP_TUNING_BRIGHTNESS
    unsigned char brightness_g, brightness_s;
    brightness_s = 128;
    isp_tuning_set_brightness(fd_tuning, brightness_s);
    fprintf(stderr, "----Set_brightness, brightness = %u\n", brightness_s);

    isp_tuning_get_brightness(fd_tuning, &brightness_g);
    fprintf(stderr, "----Get_brightness, brightness = %u\n\n", brightness_g);
#endif

#if TEST_ISP_TUNING_CONTRAST
    unsigned char contrast_g, contrast_s;
    contrast_s = 128;
    isp_tuning_set_contrast(fd_tuning, contrast_s);
    fprintf(stderr, "----Set_contrast, contrast = %u\n", contrast_s);

    isp_tuning_get_contrast(fd_tuning, &contrast_g);
    fprintf(stderr, "----Get_contrast, contrast = %u\n\n", contrast_g);
#endif

#if TEST_ISP_TUNING_SATURATION
    unsigned char saturation_g, saturation_s;
    saturation_s = 128;
    isp_tuning_set_saturation(fd_tuning, saturation_s);
    fprintf(stderr, "----Set_saturation, saturation = %u\n", saturation_s);

    isp_tuning_get_saturation(fd_tuning, &saturation_g);
    fprintf(stderr, "----Get_saturation, saturation = %u\n\n", saturation_g);
#endif

#if TEST_ISP_TUNING_SHARPNESS
    unsigned char sharpness_g, sharpness_s;
    sharpness_s = 128;
    isp_tuning_set_sharpness(fd_tuning, sharpness_s);
    fprintf(stderr, "----Set_sharpness, sharpness = %u\n", sharpness_s);

    isp_tuning_get_sharpness(fd_tuning, &sharpness_g);
    fprintf(stderr, "----Get_sharpness, sharpness = %u\n\n", sharpness_g);
#endif

#if TEST_ISP_TUNING_ANTI_FLICKER_ATTR
    enum v4l2_power_line_frequency freq;
    isp_tuning_set_anti_flicker_attr(fd_tuning, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);
    isp_tuning_get_anti_flicker_attr(fd_tuning, &freq);
    fprintf(stderr, "----Get_anti_flicker_attr, power_line_frequency = %d\n\n", freq);
#endif

#if TEST_ISP_TUNING_EV_ATTR
    isp_ev_attr ev_attr;
    isp_tuning_get_ev_attr(fd_tuning, &ev_attr);
    fprintf(stderr, "----Get_EV_attr\n");
    fprintf(stderr, "\t ae manual:                  %u\n", ev_attr.ae_manual);
    fprintf(stderr, "\t exposure value:             %u\n", ev_attr.ev);
    fprintf(stderr, "\t integration time:           %u\n", ev_attr.integration_time);
    fprintf(stderr, "\t min integration time:       %u\n", ev_attr.min_integration_time);
    fprintf(stderr, "\t max integration time:       %u\n", ev_attr.max_integration_time);
    fprintf(stderr, "\t integration time (unit:us): %u\n", ev_attr.integration_time_us);
    fprintf(stderr, "\t sensor again:               %u\n", ev_attr.sensor_again);
    fprintf(stderr, "\t max sensor again:           %u\n", ev_attr.max_sensor_again);
    fprintf(stderr, "\t sensor dgain:               %u\n", ev_attr.sensor_dgain);
    fprintf(stderr, "\t max sensor dgain:           %u\n", ev_attr.max_sensor_dgain);
    fprintf(stderr, "\t total gain:                 %u\n\n", ev_attr.total_gain);
#endif

#if TEST_ISP_TUNING_EXPR
    isp_expr expr_attr_g, expr_attr_s;
    int integration_unit = ISP_CORE_INTEGRATION_TIME_UNIT_LINE;

    expr_attr_s.mode = ISP_CORE_EXPR_MODE_MANUAL;
    expr_attr_s.again = 2048;
    expr_attr_s.integration_time.time = 801;
    expr_attr_s.integration_time.unit = integration_unit;
    isp_tuning_set_expr(fd_tuning, &expr_attr_s);
    fprintf(stderr, "----Set_expr\n");
    fprintf(stderr, "\t mode :            %d (0:auto; 1:manual)\n", expr_attr_s.mode);
    fprintf(stderr, "\t again:            %u \n", expr_attr_s.again);
    fprintf(stderr, "\t integration time: %u, unit: %d (0:line; 1:us)\n", expr_attr_s.integration_time.time, expr_attr_s.integration_time.unit);
    sleep(1);
    expr_attr_g.integration_time.unit = integration_unit;
    isp_tuning_get_expr(fd_tuning, &expr_attr_g);
    fprintf(stderr, "----Get_expr\n");
    fprintf(stderr, "\t mode:             %d (0:auto; 1:manual)\n", expr_attr_g.mode);
    fprintf(stderr, "\t again:            %u \n", expr_attr_g.again);
    fprintf(stderr, "\t integration time: %u, unit: %d (0:line; 1:us)\n\n", expr_attr_g.integration_time.time, expr_attr_g.integration_time.unit);
#endif

#if TEST_ISP_TUNING_MAX_AGAIN
    uint32_t max_gain_g, max_gain_s;
    max_gain_s = 8192;
    isp_tuning_set_max_again(fd_tuning, max_gain_s);
    fprintf(stderr, "----Set_max_again, max_again  = %u\n", max_gain_s);

    isp_tuning_get_max_again(fd_tuning, &max_gain_g);
    fprintf(stderr, "----Get_max_again, max_again  = %u\n\n", max_gain_g);
#endif

#if TEST_ISP_TUNING_AE_MIN
    isp_ae_min ae_min_g, ae_min_s;
    ae_min_s.min_again = 1024;
    ae_min_s.min_it = 256;
    isp_tuning_set_ae_min(fd_tuning, &ae_min_s);
    fprintf(stderr, "----Set_ae_min\n");
    fprintf(stderr, "\t min again            = %u\n", ae_min_s.min_again);
    fprintf(stderr, "\t min integration time = %u\n",ae_min_s.min_it);

    isp_tuning_get_ae_min(fd_tuning, &ae_min_g);
    fprintf(stderr, "----Get_ae_min\n");
    fprintf(stderr, "\t min again            = %u\n", ae_min_g.min_again);
    fprintf(stderr, "\t min integration time = %u\n\n",ae_min_g.min_it);
#endif

#if TEST_ISP_TUNING_AE_LUMA
    int luma;
    isp_tuning_get_ae_luma(fd_tuning, &luma);
    fprintf(stderr, "----Get_ae_luma, luma = %u\n\n", luma);
#endif

#if TEST_ISP_TUNING_HI_LIGHT_DEPRESS
    uint32_t strength_g, strength_s;
    strength_s = 5;
    isp_tuning_set_hi_light_depress(fd_tuning, strength_s);
    fprintf(stderr, "----Set_high_light_depress, strength = %u\n", strength_s);

    isp_tuning_get_hi_light_depress(fd_tuning, &strength_g);
    fprintf(stderr, "----Get_high_light_depress, strength = %u\n\n", strength_g);
#endif

#if TEST_ISP_TUNING_AE_WEIGHT
    isp_weight ae_weight_g;
    isp_weight ae_weight_s = { //15*15各区域权重取值范围[0-8]
        .weight = {
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
        }
    };
    isp_tuning_set_ae_weight(fd_tuning, &ae_weight_s);
    sleep(1);

    isp_tuning_get_ae_weight(fd_tuning, &ae_weight_g);
    fprintf(stderr, "----Get_ae_weight[15][15] \n\t");
    fprintf(stderr, "    00 01 02 03 04 05 06 07 08 09 10 11 12 13 14\n\t");
    for (int i = 0; i < 15; i++) {
        fprintf(stderr, "%02d:", i);
        for (int j = 0; j < 15; j++) {
            fprintf(stderr, "  %u", ae_weight_s.weight[i][j]);
        }
        fprintf(stderr, "\n\t");
    }
    fprintf(stderr, "\n");
#endif

#if TEST_ISP_TUNING_WB
    isp_wb wb_attr_g, wb_attr_s;
    wb_attr_s.mode = ISP_CORE_WB_MODE_MANUAL;
    wb_attr_s.rgain = 400;
    wb_attr_s.bgain = 400;
    isp_tuning_set_wb(fd_tuning, &wb_attr_s);
    fprintf(stderr, "----Set_wb, mode = %d, rgain = %u, bgain = %u\n", wb_attr_s.mode, wb_attr_s.rgain, wb_attr_s.bgain);

    sleep(1);
    isp_tuning_get_wb(fd_tuning, &wb_attr_g);
    fprintf(stderr, "----Get_wb, mode = %d, rgain = %u, bgain = %u\n\n", wb_attr_g.mode, wb_attr_g.rgain, wb_attr_g.bgain);
#endif

#if TEST_ISP_TUNING_GAMMA
    isp_gamma gamma_g;
    isp_gamma gamma_s = {
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
    fprintf(stderr, "----Get_gamma[129]\n\t");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 16; j++) {
            fprintf(stderr, "%04u ", gamma_g.gamma[i*16+j]);
        }
        fprintf(stderr, "\n\t");
    }
    fprintf(stderr, "%u\n", gamma_g.gamma[128]);
#endif

#if TEST_ISP_TUNING_ADR_STRENGTH
    uint32_t adr_strength_g, adr_strength_s;
    adr_strength_s = 30;
    isp_tuning_set_adr_strength(fd_tuning, adr_strength_s);
    fprintf(stderr, "----Set_adr_strength, strength = %u\n", adr_strength_s);

    isp_tuning_get_adr_strength(fd_tuning, &adr_strength_g);
    fprintf(stderr, "----Get_adr_strength, strength = %u\n\n", adr_strength_g);
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
