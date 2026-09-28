/*
 * isp tuning header file.
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __ISPTuning_H__
#define __ISPTuning_H__

#include <stdint.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * ISP图像信号处理。主要包含图像效果设置操作，与数据流无关，仅作用于效果参数设置及Sensor控制。
 *
 */


#define VIDIOC_PRIVATE_G_CTRL     _IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct v4l2_control)
#define VIDIOC_PRIVATE_S_CTRL     _IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct v4l2_control)




/**
 * ISP功能开关
 */
typedef enum {
    ISP_TUNING_OPS_MODE_DISABLE,            /**< 不使能该模块功能 */
    ISP_TUNING_OPS_MODE_ENABLE,            /**< 使能该模块功能 */
    ISP_TUNING_OPS_MODE_BUTT,            /**< 用于判断参数的有效性，参数大小必须小于这个值 */
} isp_tuning_ops_mode;

/**
 * ISP功能选用开关
 */
typedef enum {
    ISP_TUNING_OPS_TYPE_AUTO,            /**< 该模块的操作为自动模式 */
    ISP_TUNING_OPS_TYPE_MANUAL,            /**< 该模块的操作为手动模式 */
    ISP_TUNING_OPS_TYPE_BUTT,            /**< 用于判断参数的有效性，参数大小必须小于这个值 */
} isp_tuning_ops_type;




/**
 * @fn int isp_tuning_open(const char *dev)
 *
 * 打开isp tuning设备
 *
 * @param[in] dev isp tuning设备名
 *
 * @retval >0 成功，返回tuning句柄
 * @retval <0 失败
 *
 * @attention 在使用这个函数之前，必须保证ISP stream on。
 */
int isp_tuning_open(const char *dev);

/**
 * @fn int isp_tuning_close(int fd)
 *
 * 关闭isp tuning设备
 *
 * @param[in] fd 要关闭的设备
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 */
int isp_tuning_close(int fd);

/**
 * ISP Module Control
 */
typedef union {
    unsigned int key;
    struct {
        unsigned int bitBypassDPC : 1; /* [0]  */
        unsigned int bitBypassGIB : 1; /* [1]  */
        unsigned int bitBypassLSC : 1; /* [2]  */
        unsigned int bitBypassAWB : 1; /* [3]  */
        unsigned int bitBypassADR : 1; /* [4]  */
        unsigned int bitBypassDMSC : 1; /* [5]    */
        unsigned int bitBypassCCM : 1; /* [6]  */
        unsigned int bitBypassGAMMA : 1; /* [7]     */
        unsigned int bitBypassDEFOG : 1; /* [8]     */
        unsigned int bitBypassCLM : 1; /* [9]  */
        unsigned int bitBypassYSHARPEN : 1; /* [10]  */
        unsigned int bitBypassMDNS : 1; /* [11]     */
        unsigned int bitBypassSDNS : 1; /* [12]     */
        unsigned int bitBypassHLDC : 1; /* [13]     */
        unsigned int bitBypassTP : 1; /* [14]  */
        unsigned int bitBypassFONT : 1; /* [15]     */
        unsigned int bitRsv : 15; /* [16 ~ 30]    */
        unsigned int bitRsv2 : 1; /* [31]  */
    };
} isp_module_ctrl;

/**
 * @fn int isp_tuning_set_module_control(int fd, isp_module_ctrl *isp_module)
 *
 * 设置ISP各个模块bypass功能
 *
 * @param[in] ispmodule ISP各个模块bypass功能.
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_module_control(int fd, isp_module_ctrl *isp_module);

/**
 * @fn int isp_tuning_get_module_control(int fd, isp_module_ctrl *isp_module)
 *
 * 获取ISP各个模块bypass功能.
 *
 * @param[out] ispmodule ISP各个模块bypass功能
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_module_control(int fd, isp_module_ctrl *isp_module);

/**
 * ISP 工作模式配置，正常模式或夜视模式。
 */
typedef enum {
    ISP_RUNNING_MODE_DAY = 0,                /**< 正常模式 */
    ISP_RUNNING_MODE_NIGHT = 1,                /**< 夜视模式 */
    ISP_RUNNING_MODE_BUTT,                    /**< 最大值 */
} isp_running_mode;

/**
 * @fn int isp_tuning_get_isp_running_mode(int fd, isp_running_mode *mode)
 *
 * 获取ISP工作模式，正常模式或夜视模式。
 *
 * @param[in] mode操作参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_isp_running_mode(int fd, isp_running_mode *mode);

/**
 * @fn int isp_tuning_set_isp_running_mode(int fd, isp_running_mode mode)
 *
 * 设置ISP工作模式，正常模式或夜视模式；默认为正常模式。
 *
 * @param[in] mode运行模式参数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_isp_running_mode(int fd, isp_running_mode mode);

/**
 * @fn int isp_tuning_get_isp_hflip(int fd, isp_tuning_ops_mode *mode)
 *
 * 获取ISP图像镜面效果功能的操作状态
 *
 * @param[in] pmode 操作参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_isp_hflip(int fd, isp_tuning_ops_mode *mode);

/**
 * 设置ISP图像镜面效果功能是否使能
 *
 * @fn int isp_tuning_set_isp_hflip(int fd, isp_tuning_ops_mode mode)
 *
 * @param[in] mode 是否使能镜面效果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_isp_hflip(int fd, isp_tuning_ops_mode mode);

/**
 * @fn int isp_tuning_get_isp_vflip(int fd, isp_tuning_ops_mode *mode)
 *
 * 获取ISP图像上下反转效果功能的操作状态
 *
 * @param[in] pmode 操作参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_isp_vflip(int fd, isp_tuning_ops_mode *mode);

/**
 * @fn int isp_tuning_set_isp_vflip(int fd, isp_tuning_ops_mode mode)
 *
 * 设置ISP图像上下反转效果功能是否使能
 *
 * @param[in] mode 是否使能图像上下反转
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_isp_vflip(int fd, isp_tuning_ops_mode mode);

/**
 * @fn int isp_tuning_get_sensor_fps(int fd, uint32_t *fps_num, uint32_t *fps_den)
 *
 * 获取摄像头输出帧率
 *
 * @param[in] fps_num 获取帧率分子参数的指针
 * @param[in] fps_den 获取帧率分母参数的指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_sensor_fps(int fd, uint32_t *fps_num, uint32_t *fps_den);

/**
 * @fn int isp_tuning_set_sensor_fps(int fd, uint32_t fps_num, uint32_t fps_den)
 *
 * 设置摄像头输出帧率
 *
 * @param[in] fps_num 设定帧率的分子参数
 * @param[in] fps_den 设定帧率的分母参数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_sensor_fps(int fd, uint32_t fps_num, uint32_t fps_den);

/**
 * @fn int isp_tuning_get_brightness(int fd, unsigned char *brightness)
 *
 * 获取ISP 综合效果图片亮度
 *
 * @param[in] bright 图片亮度参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加亮度，小于128降低亮度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_get_brightness(int fd, unsigned char *brightness);

/**
 * @fn int isp_tuning_set_brightness(int fd, unsigned char brightness)
 *
 * 设置ISP 综合效果图片亮度
 *
 * @param[in] brightness 图片亮度参数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加亮度，小于128降低亮度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_set_brightness(int fd, unsigned char brightness);

/**
 * @fn int isp_tuning_get_contrast(int fd, unsigned char *contrast)
 *
 * 获取ISP 综合效果图片对比度
 *
 * @param[in] contrast 图片对比度参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加对比度，小于128降低对比度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_get_contrast(int fd, unsigned char *contrast);

/**
 * @fn int isp_tuning_set_contrast(int fd, unsigned char contrast)
 *
 * 设置ISP 综合效果图片对比度
 *
 * @param[in] contrast 图片对比度参数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加对比度，小于128降低对比度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_set_contrast(int fd, unsigned char contrast);

/**
 * @fn int isp_tuning_get_saturation(int fd, unsigned char *saturation)
 *
 * 获取ISP 综合效果图片饱和度
 *
 * @param[in] sat 图片饱和度参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加饱和度，小于128降低饱和度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_get_saturation(int fd, unsigned char *saturation);

/**
 * @fn int isp_tuning_set_saturation(int fd, unsigned char saturation)
 *
 * 设置ISP 综合效果图片饱和度
 *
 * @param[in] sat 图片饱和度参数值
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加饱和度，小于128降低饱和度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_set_saturation(int fd, unsigned char saturation);

/**
 * @fn int isp_tuning_get_sharpness(int fd, unsigned char *sharpness)
 *
 * 获取ISP 综合效果图片锐度
 *
 * @param[in] sharpness 图片锐度参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加锐度，小于128降低锐度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_get_sharpness(int fd, unsigned char *sharpness);

/**
 * @fn int isp_tuning_set_sharpness(int fd, unsigned char sharpness)
 *
 * 设置ISP 综合效果图片锐度
 *
 * @param[in] sharpness 图片锐度参数值
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 默认值为128，大于128增加锐度，小于128降低锐度。
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_set_sharpness(int fd, unsigned char sharpness);

/**
 * @fn int isp_tuning_get_anti_flicker_attr(int fd, enum v4l2_power_line_frequency *attr)
 *
 * 获得ISP抗闪频属性
 *
 * @param[in] attr 获取参数值指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_get_anti_flicker_attr(int fd, enum v4l2_power_line_frequency *attr);

/**
 * @fn int isp_tuning_set_anti_flicker_attr(int fd, enum v4l2_power_line_frequency attr)
 *
 * 设置ISP抗闪频属性
 *
 * @param[in] attr 设置参数值
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图.
 */
int isp_tuning_set_anti_flicker_attr(int fd, enum v4l2_power_line_frequency attr);

/**
 * ISP EV 参数。
 */
typedef struct {
    unsigned int ae_manual;                 /**< 曝光模式 */
    unsigned int ev;                        /**< 曝光值 */
    unsigned int integration_time;          /**< 曝光时间 */
    unsigned int min_integration_time;      /**< 最小曝光时间 */
    unsigned int max_integration_time;      /**< 最大曝光时间 */
    unsigned int integration_time_us;       /**< 曝光时间，单位微秒 */
    unsigned int sensor_again;              /**< 模拟增益，1024表示1x，2048表示2x，依次类推 */
    unsigned int max_sensor_again;          /**< 最大模拟增益 */
    unsigned int sensor_dgain;              /**< sensor数字增益 */
    unsigned int max_sensor_dgain;          /**< 最大sensor数字增益 */
    unsigned int isp_dgain;                 /**< isp数字增益 */
    unsigned int max_isp_dgain;             /**< 最大isp数字增益 */
    unsigned int total_gain;                /**< 总增益增益 */
} isp_ev_attr;

/**
* @fn int isp_tuning_get_ev_attr(int fd, isp_ev_attr *attr)
*
* 获取EV属性。
* @param[out] attr EV属性参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_ev_attr(int fd, isp_ev_attr *attr);

/**
 * 曝光模式
 */
enum isp_core_expr_mode {
    ISP_CORE_EXPR_MODE_AUTO = 0,            /**< 自动模式 */
    ISP_CORE_EXPR_MODE_MANUAL,              /**< 手动模式 */
};

/**
 * 曝光单位
 */
enum isp_core_integration_time_unit {
    ISP_CORE_INTEGRATION_TIME_UNIT_LINE,    /**< 行 */
    ISP_CORE_INTEGRATION_TIME_UNIT_US,      /**< 微秒 */
};

/**
 * 曝光时间
 */
struct isp_core_integration_time {
    enum isp_core_integration_time_unit unit;
    unsigned int time;
};

/**
 * 曝光参数
 */
typedef struct isp_core_expr_attr{
    enum isp_core_expr_mode mode;
    struct isp_core_integration_time integration_time;  /** 曝光时间 */
    unsigned int again;                                 /** 模拟增益，1024表示1x，2048表示2x，依次类推*/
} isp_expr;

/**
 * @fn int isp_tuning_get_expr(int fd, isp_expr *expr)
 *
 * 获取AE参数。
 *
 * @param[out] expr AE参数。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_expr(int fd, isp_expr *expr);

/**
 * @fn int isp_tuning_set_expr(int fd, isp_expr *expr)
 *
 * 设置AE参数。
 *
 * @param[in] expr AE参数。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_expr(int fd, isp_expr *expr);

/**
 * @fn int isp_tuning_get_max_again(int fd, uint32_t *gain)
 *
 * 获取sensor可以设置最大Again。
 *
 * @param[out] gain sensor可以设置的最大again.1024表示1x，2048表示2x，依次类推。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_max_again(int fd, uint32_t *gain);

/**
 * @fn int isp_tuning_set_max_again(int fd, uint32_t gain)
 *
 * 设置sensor可以设置最大Again。
 *
 * @param[in] gain sensor可以设置的最大again.1024表示1x，2048表示2x，依次类推。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_max_again(int fd, uint32_t gain);

/**
 * @fn int isp_tuning_get_max_dgain(int fd, uint32_t *gain)
 *
 * 获取ISP设置的最大Dgain。
 *
 * @param[out] ISP Dgain 可以得到设置的最大的dgain.1024表示1x，2048表示2x，依次类推。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_max_dgain(int fd, uint32_t *gain);

/**
 * @fn int isp_tuning_set_max_dgain(int fd, uint32_t gain)
 *
 * 设置ISP可以设置的最大Dgain。
 *
 * @param[in] ISP Dgain 可以设置的最大dgain.1024表示1x，2048表示2x，依次类推。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_max_dgain(int fd, uint32_t gain);

/**
 * @fn int isp_tuning_get_total_gain(int fd, uint32_t *gain)
 *
 * 获取ISP输出图像的整体增益值
 *
 * @param[in] gain 获取增益值参数的指针,1024表示1x，2048表示2x，依次类推。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_total_gain(int fd, uint32_t *gain);

/**
 * AE Min
 */
typedef struct {
    unsigned int min_it;  /**< AE最小曝光 */
    unsigned int min_again;     /**< AE 最小模拟增益 */
} isp_ae_min;

/**
 * @fn int isp_tuning_get_ae_min(int fd, isp_ae_min *ae_min)
 *
 * 获取AE最小值参数。
 *
 * @param[out] ae_min AE最小值信息。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_ae_min(int fd, isp_ae_min *ae_min);

/**
 * @fn int isp_tuning_set_ae_min(int fd, isp_ae_min *ae_min)
 *
 * 设置AE最小值参数。
 *
 * @param[in] ae_min AE最小值参数。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_ae_min(int fd, isp_ae_min *ae_min);

/**
* @fn int isp_tuning_get_ae_luma(int fd, int *luma)
*
* 获取画面平均亮度。
*
* @param[out] luma AE亮度参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_ae_luma(int fd, int *luma);

/**
 * @fn int isp_tuning_get_hi_light_depress(int fd, uint32_t *strength)
 *
 * 获取强光抑制的强度。
 *
 * @param[out] strength 可以得到设置的强光抑制的强度.0表示关闭此功能。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_hi_light_depress(int fd, uint32_t *strength);

/**
 * @fn int isp_tuning_set_hi_light_depress(int fd, uint32_t strength)
 *
 * 设置强光抑制强度。
 *
 * @param[in] strength 强光抑制强度参数.取值范围为［0-10], 0表示关闭功能。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_hi_light_depress(int fd, uint32_t strength);


typedef struct {
    unsigned int zone[15][15];    /**< 各区域信息*/
} isp_zone;

/**
 * @fn int isp_tuning_get_ae_zone(int fd, isp_zone *ae_zone)
 *
 * 获取AE各个zone的Y值。
 *
 * @param[out] ae_zone AE各个区域的Y值。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_ae_zone(int fd, isp_zone *ae_zone);

/**
* AE统计值参数
*/
typedef struct {
    unsigned char ae_histhresh[4];    /**< AE统计直方图bin边界 [0 ~ 255]*/
    unsigned short ae_hist[5];    /**< AE统计直方图bin值 [0 ~ 65535]*/
    unsigned char ae_stat_nodeh;    /**< 水平方向有效统计区域个数 [0 ~ 15]*/
    unsigned char ae_stat_nodev;    /**< 垂直方向有效统计区域个数 [0 ~ 15]*/
} isp_ae_hist;

/**
 * @fn int isp_tuning_get_ae_hist(int fd, isp_ae_hist *ae_hist)
 *
 * 获取AE统计值。
 *
 * @param[out] ae_hist AE统计值信息。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_ae_hist(int fd, isp_ae_hist *ae_hist);

/**
 * @fn int isp_tuning_set_ae_hist(int fd, isp_ae_hist *ae_hist)
 *
 * 设置AE统计相关参数。
 *
 * @param[in] ae_hist AE统计相关参数。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_ae_hist(int fd, isp_ae_hist *ae_hist);

/**
* 权重信息
*/
typedef struct {
    unsigned char weight[15][15];    /**< 各区域权重信息 [0 ~ 8]*/
} isp_weight;

/**
 * @fn int isp_tuning_get_ae_roi(int fd, isp_weight *roi_weight)
 *
 * 获取AE感兴趣区域，用于场景判断。
 *
 * @param[out] roi_weight AE感兴趣区域权重。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_ae_roi(int fd, isp_weight *roi_weight);

/**
 * @fn int isp_tuning_set_ae_roi(int fd, isp_weight *roi_weight)
 *
 * 获取AE感兴趣区域，用于场景判断。
 *
 * @param[in] roi_weight AE感兴趣区域权重。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_ae_roi(int fd, isp_weight *roi_weight);

/**
 * @fn int isp_tuning_get_ae_weight(int fd, isp_weight *ae_weight)
 *
 * 获取AE统计区域的权重。
 *
 * @param[out] ae_weight 各区域权重信息。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_ae_weight(int fd, isp_weight *ae_weight);

/**
 * @fn int isp_tuning_set_ae_weight(int fd, isp_weight *ae_weight)
 *
 * 设置AE统计区域的权重。
 *
 * @param[in] ae_weight 各区域权重信息。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_ae_weight(int fd, isp_weight *ae_weight);

/**
 * 白平衡模式
 */
enum isp_core_wb_mode {
    ISP_CORE_WB_MODE_AUTO = 0,            /**< 自动模式 */
    ISP_CORE_WB_MODE_MANUAL,            /**< 手动模式 */
    ISP_CORE_WB_MODE_DAY_LIGHT,            /**< 晴天 */
    ISP_CORE_WB_MODE_CLOUDY,            /**< 阴天 */
    ISP_CORE_WB_MODE_INCANDESCENT,        /**< 白炽灯 */
    ISP_CORE_WB_MODE_FLOURESCENT,        /**< 荧光灯 */
    ISP_CORE_WB_MODE_TWILIGHT,            /**< 黄昏 */
    ISP_CORE_WB_MODE_SHADE,                /**< 阴影 */
    ISP_CORE_WB_MODE_WARM_FLOURESCENT,    /**< 暖色荧光灯 */
    ISP_CORE_WB_MODE_CUSTOM,    /**< 自定义模式 */
};

/**
 * 白平衡参数
 */
typedef struct isp_core_wb_attr{
    enum isp_core_wb_mode mode;        /**< 白平衡模式，分为自动与手动模式 */
    uint16_t rgain;            /**< 红色增益，手动模式时有效 */
    uint16_t bgain;            /**< 蓝色增益，手动模式时有效 */
} isp_wb;

/**
 * @fn int isp_tuning_set_wb(int fd, isp_wb *wb)
 *
 * 设置白平衡功能设置。可以设置自动与手动模式，手动模式主要通过设置rgain、bgain实现。
 *
 * @param[in] wb 设置的白平衡参数。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_wb(int fd, isp_wb *wb);

/**
 * @fn int isp_tuning_get_wb(int fd, isp_wb *wb)
 *
 * 获取白平衡功能设置。
 *
 * @param[out] wb 获取的白平衡参数。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_wb(int fd, isp_wb *wb);

/**
 * @fn int isp_tuning_get_wb_statis(int fd, isp_wb *wb)
 *
 * 获取白平衡统计值。
 *
 * @param[out] wb 获取的白平衡统计值。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_wb_statis(int fd, isp_wb *wb);

/**
 * @fn int isp_tuning_get_wb_gol_statis(int fd, isp_wb *wb)
 *
 * 获取白平衡全局统计值。
 *
 * @param[out] wb 获取的白平衡全局统计值。
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_wb_gol_statis(int fd, isp_wb *wb);

/**
 * gamma
 */
typedef struct {
    uint16_t gamma[129];        /**< gamma参数数组，有129个点 */
} isp_gamma;

/**
* @fn int isp_tuning_get_gamma(int fd, isp_gamma *gamma)
*
* 获取GAMMA参数.
* @param[out] gamma gamma参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_gamma(int fd, isp_gamma *gamma);

/**
* @fn int isp_tuning_set_gamma(int fd, isp_gamma *gamma)
*
* 设置GAMMA参数.
* @param[in] gamma gamma参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_gamma(int fd, isp_gamma *gamma);

/**
 * @fn int isp_tuning_get_adr_strength(int fd, uint32_t *strength)
 *
 * 获取ADR强度值.
 *
 * @param[out] strength DRC强度.
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_adr_strength(int fd, uint32_t *strength);

/**
 * @fn int isp_tuning_set_adr_strength(int fd, uint32_t strength)
 *
 * 设置ADR强度值.
 *
 * @param[in] strength DRC强度.
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_adr_strength(int fd, uint32_t strength);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* __ISPTuning_H__ */
