/*
 * isp tuning header file.
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __ISPTuning_x2580_H__
#define __ISPTuning_x2580_H__

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
    ISP_TUNING_OPS_MODE_DISABLE,                /**< 不使能该模块功能 */
    ISP_TUNING_OPS_MODE_ENABLE,                 /**< 使能该模块功能 */
    ISP_TUNING_OPS_MODE_BUTT,                   /**< 用于判断参数的有效性，参数大小必须小于这个值 */
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
 *  ISP各个模块旁路开关
 */
typedef union {
    uint32_t key;                          /**< 各个模块旁路开关 */
    struct {
        uint32_t bitBypassBLC : 1;      /**< [0] */
        uint32_t bitBypassLSC : 1;      /**< [1] */
        uint32_t bitBypassAWB0 : 1;     /**< [2] */
        uint32_t bitBypassWDR : 1;      /**< [3] */
        uint32_t bitBypassDPC : 1;      /**< [4] */
        uint32_t bitBypassGIB : 1;      /**< [5] */
        uint32_t bitBypassAWB1 : 1;     /**< [6] */
        uint32_t bitBypassADR : 1;      /**< [7] */
        uint32_t bitBypassDMSC : 1;     /**< [8] */
        uint32_t bitBypassCCM : 1;      /**< [9] */
        uint32_t bitBypassGAMMA : 1;    /**< [10] */
        uint32_t bitBypassDEFOG : 1;    /**< [11] */
        uint32_t bitBypassCSC : 1;      /**< [12] */
        uint32_t bitBypassMDNS : 1;     /**< [13] */
        uint32_t bitBypassYDNS : 1;     /**< [14] */
        uint32_t bitBypassBCSH : 1;     /**< [15] */
        uint32_t bitBypassCLM : 1;      /**< [16] */
        uint32_t bitBypassYSP : 1;      /**< [17] */
        uint32_t bitBypassSDNS : 1;     /**< [18] */
        uint32_t bitBypassCDNS : 1;     /**< [19] */
        uint32_t bitBypassHLDC : 1;     /**< [20] */
        uint32_t bitBypassLCE : 1;      /**< [21] */
        uint32_t bitBypassTMO : 1;      /**< [22] */
        uint32_t bitBypassIRDPC : 1;    /**< [23] */
        uint32_t bitBypassIRGAMMA : 1;  /**< [24] */
        uint32_t bitRsv : 7;            /**< [25 ~ 31] */
    };
} ISPModuleCtl;

/**
 * @fn int isp_tuning_set_module_control(int fd, ISPModuleCtl *isp_module)
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
int isp_tuning_set_module_control(int fd, ISPModuleCtl *isp_module);

/**
 * @fn int isp_tuning_get_module_control(int fd, ISPModuleCtl *isp_module)
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
int isp_tuning_get_module_control(int fd, ISPModuleCtl *isp_module);

/**
 * ISP 工作模式配置，正常模式或夜视模式。
 */
typedef enum {
    TISP_RUNING_MODE_DAY_MODE,
    TISP_RUNING_MODE_NIGHT_MODE,
    TISP_RUNING_MODE_CUSTOM_MODE,
    TISP_RUNING_MODE_BUTT,
} isp_running_mode;

typedef enum {
    ISP_FLIP_NORMAL_MODE = 0,
    ISP_FLIP_ISP_H_MODE,
    ISP_FLIP_ISP_V_MODE,
    ISP_FLIP_ISP_HV_MODE,
    ISP_FLIP_MODE_BUTT,
} ISP_CORE_HVFLIP;

typedef struct {
    ISP_CORE_HVFLIP sensor_mode;    /**< sensor驱动中的hvflip函数实现 */
    ISP_CORE_HVFLIP isp_mode[3];    /**< ISP实现，isp_mode[n]对应mscaler通道n */
    /**< sensor_mode 和 isp_mode[]不能同时设定 */
} tisp_hv_flip_t;

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
 * @fn int isp_tuning_set_isp_running_mode(int fd, isp_running_mode *mode)
 *
 * 设置ISP工作模式，正常模式或夜视模式；默认为正常模式。
 *
 * @param[in] mode操作参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_isp_running_mode(int fd, isp_running_mode *mode);

/**
 * @fn int isp_tuning_get_isp_hvflip(int fd, isp_tuning_ops_mode *mode)
 *
 * 获取ISP图像镜面效果功能的操作状态
 *
 * @param[in] mode 操作参数指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_get_isp_hvflip(int fd, tisp_hv_flip_t *mode);

/**
 * 设置ISP图像镜面效果功能是否使能
 *
 * @fn int isp_tuning_set_isp_hvflip(int fd, isp_tuning_ops_mode *mode)
 *
 * @param[in] mode 是否使能镜面效果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @attention 在使用这个函数之前，必须保证ISP出图。
 */
int isp_tuning_set_isp_hvflip(int fd, tisp_hv_flip_t *mode);


/**
 * ISP CSC转换矩阵标准与模式结构体
 */
typedef enum {
    ISP_CG_BT601_FULL,          /**< BT601 full range */
    ISP_CG_BT601_LIMITED,       /**< BT601 非full range */
    ISP_CG_BT709_FULL,          /**< BT709 full range */
    ISP_CG_BT709_LIMITED,       /**< BT709 非full range */
    ISP_CG_USER,                /**< 用户自定义模式 */
    ISP_CG_BUTT,                /**< 用于判断参数的有效性，参数大小必须小于这个值 */
} ISPCSCColorGamut;

/**
 * ISP CSC转换矩阵结构体
 */
typedef struct {
    float CscCoef[9];               /**< 3x3矩阵 */
    unsigned char CscOffset[2];     /**< [0] UV偏移值 [1] Y偏移值*/
    unsigned char CscClip[4];       /**< 分别为Y最大值，Y最大值，UV最大值，UV最小值 */
} ISPCscMatrix;

/**
 * ISP CSC属性结构体
 */
typedef struct {
    ISPCSCColorGamut ColorGamut;     /**< RGB转YUV的标准矩阵 */
    ISPCscMatrix Matrix;             /**< 客户自定义的转换矩阵 */
} isp_csc_attr;

/**
 * @fn int32_t isp_tuning_set_isp_csc_attr(int fd, isp_csc_attr *csc)
 *
 * 设置CSC属性.
 *
 * @param[in] fd 对应sensor的标号
 * @param[in] csc_attr CSC属性参数.
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * int ret = 0;
 * isp_csc_attr csc_attr;
 *
 * memset(&csc_attr, 0, sizeof(csc_attr));
 * csc_attr.ColorGamut = mode;
 *
 * if (csc_attr.ColorGamut == ISP_CG_USER) {
 *      float CscCoef[] = {...};
 *      unsigned char CscOffset[] = {...};
 *      unsigned char CscClip[] = {...};
 *
 *      memcpy(csc_attr.CscCoef, CscCoef, sizeof(CscCoef));
 *      memcpy(csc_attr.CscOffset, CscOffset, sizeof(CscOffset));
 *      memcpy(csc_attr.CscClip, CscClip, sizeof(CscClip));
 * }
 * ret = isp_tuning_set_isp_csc_attr(fd, &csc_attr);
 * if(ret){
 *     printf("isp_tuning_set_isp_csc_attr error !\n");
 *     return -1;
 * }
 * @endcode
 *
 * @attention
 */
int32_t isp_tuning_set_isp_csc_attr(int fd, isp_csc_attr *csc_attr);

/**
 * @fn int32_t isp_tuning_get_isp_csc_attr(int fd, isp_csc_attr *csc_attr)
 *
 * 获取CSC属性.
 *
 * @param[in] num       对应sensor的标号
 * @param[out] csc_attr      CSC属性参数.
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * int ret = 0;
 * isp_csc_attr csc_attr;
 *
 * memset(&csc, 0, sizeof(csc_attr));
 *
 * ret = isp_tuning_get_isp_csc_attr(fd, &csc_attr);
 * if(ret){
 *     printf("isp_tuning_get_isp_csc_attr error !\n");
 *     return -1;
 * }
 * printf("csc_attr color gamut:%d\n", csc_attr.ColorGamut);
 * if (csc_attr.ColorGamut == ISP_CG_USER) {
 *      printf("CscCoef:%f ..., CscOffset:%d ..., CscClip:%d ...\n", ...);
 * }
 * @endcode
 *
 * @attention
 */
int32_t isp_tuning_get_isp_csc_attr(int fd, isp_csc_attr *csc_attr);

/**
 * ISP 颜色矩阵属性
 */
typedef struct {
    isp_tuning_ops_mode ManualEn;       /**< 手动CCM使能 */
    isp_tuning_ops_mode SatEn;          /**< 手动模式下饱和度使能 */
    float ColorMatrix[9];               /**< 颜色矩阵 */
} isp_ccm_attr;
/**
 * @fn int32_t isp_tuning_set_isp_ccm_attr(int fd, isp_ccm_attr *ccm_attr)
 *
 * 设置CCM属性.
 *
 * @param[in] fd 对应sensor的标号
 * @param[in] ccm CCM属性参数.
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * int ret = 0;
 * isp_ccm_attr ccm_attr;
 * float ColorMatrix[9] = {...};
 *
 * ccm_attr.ManualEn = ISP_TUNING_OPS_MODE_ENABLE;
 * ccm_attr.SatEn = ISP_TUNING_OPS_MODE_ENABLE;
 * memcpy(ccm_attr.ColorMatrix, ColorMatrix, sizeof(ColorMatrix));
 *
 * ret = isp_tuning_set_isp_ccm_attr(fd, &ccm_attr);
 * if(ret){
 *     printf("isp_tuning_set_isp_ccm_attr error !\n");
 *     return -1;
 * }
 * @endcode
 *
 * @attention
 */
int32_t isp_tuning_set_isp_ccm_attr(int fd, isp_ccm_attr *ccm_attr);

/**
 * @fn int32_t isp_tuning_get_isp_ccm_attr(int fd, isp_ccm_attr *ccm_attr)
 *
 * 获取CCM属性.
 *
 * @param[in] fd       对应sensor的标号
 * @param[out] ccm      CCM属性参数.
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * int ret = 0;
 * isp_ccm_attr ccm_attr;
 *
 * memset(&attr, 0x0, sizeof(isp_ccm_attr));
 * ret = isp_tuning_get_isp_ccm_attr(fd, &attr);
 * if(ret){
 *     printf("isp_tuning_get_isp_ccm_attr error !\n");
 *     return -1;
 * }
 * printf("manual_en:%d, sat_en:%d\n", ccm_attr.ManualEn, ccm_attr.SatEn);
 * printf("color matrix:...", ...);
 * @endcode
 *
 * @attention
 */
int32_t isp_tuning_get_isp_ccm_attr(int fd, isp_ccm_attr *ccm_attr);


/**
 * ISP AutoZoom Attribution
 */
typedef struct {
    int32_t zoom_chx_en[3];     /**< 数字自动对焦功能通道使能 */
    int32_t zoom_left[3];       /**< 自动对焦区域横向起始点，需要小于原始图像的宽度 */
    int32_t zoom_top[3];        /**< 自动对焦区域纵向起始点，需要小于原始图像的高度 */
    int32_t zoom_width[3];      /**< 自动对焦区域的宽度 */
    int32_t zoom_height[3];     /**< 自动对焦区域的高度 */
} isp_auto_zoom;

/**
 * @fn int32_t isp_tuning_set_isp_auto_zoom(int fd, isp_auto_zoom *ispautozoom)
 *
 * 设置自动对焦功能的属性。
 *
 * @param[in] num           对应sensor的标号
 * @param[in] ispautozoom   自动对焦功能的属性
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * //以1080P为例，ch0已开启
 * int ret = 0;
 * isp_auto_zoom zoom;
 *
 * zoom.zoom_chx_en[0] = 1;
 * zoom.zoom_left[0] = 10;
 * zoom.zoom_top[0] = 10;
 * zoom.zoom_width[0] = 640;
 * zoom.zoom_height[0] = 480;
 *
 * ret = isp_tuning_set_isp_auto_zoom(fd, &zoom);
 * if(ret){
 *     printf"isp_tuning_set_isp_auto_zoom error !\n");
 *     return -1;
 * }
 * @endcode
 *
 * @attention 在使用这个函数之前，ISP_EnableTuning已被调用。
 */
int32_t isp_tuning_set_isp_auto_zoom(int fd, isp_auto_zoom *ispautozoom);

/**
 * @fn int32_t isp_tuning_get_isp_auto_zoom(int fd, isp_auto_zoom *ispautozoom)
 *
 * 获取自动对焦功能的属性。
 *
 * @param[in] num               对应sensor的标号
 * @param[out] ispautozoom      自动对焦功能的属性
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * int i;
 * int ret = 0;
 * isp_auto_zoom zoom;
 *
 * ret = isp_tuning_get_isp_auto_zoom(fd, &zoom);
 * if(ret){
 *     printf"isp_tuning_get_isp_auto_zoom error !\n");
 *     return -1;
 * }
 * for(i=0; i<3; i++){
 *      if (zoom.zoom_chx_en[i]) {
 *              printf("ch:%d, left:%d, top:%d, width:%d, height:%d\n", i,
 *              zoom.zoom_left[i], zoom.zoom_top[i], zoom.zoom_width[i], zoom.zoom_height[i]);
 *      }
 * }
 * @endcode
 *
 * @attention 在使用这个函数之前，ISP_EnableTuning已被调用。
 */
int32_t isp_tuning_get_isp_auto_zoom(int fd, isp_auto_zoom *ispautozoom);

/**
 * Sensor帧率
 */
typedef struct {
    uint32_t num;       /**< 帧率的分子参数 */
    uint32_t den;       /**< 帧率的分母参数 */
} isp_sensor_fps;

/**
 * @fn int32_t isp_tuning_set_sensor_fps(int fd, isp_sensor_fps *fps)
 *
 * 设置摄像头输出帧率
 *
 * @param[in] num       对应sensor的标号
 * @param[in] fps       帧率属性
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * int ret = 0;
 * isp_sensor_fps fps;
 *
 * fps.num = 15;
 * fps.den = 1;
 * ret = isp_tuning_set_sensor_fps(fd, &fps);
 * if(ret){
 *     printf"isp_tuning_set_sensor_fps error !\n");
 *     return -1;
 * }
 * @endcode
 *
 * @attention
 */
int32_t isp_tuning_set_sensor_fps(int fd, isp_sensor_fps *fps);

/**
 * @fn int32_t isp_tuning_get_sensor_fps(int fd, isp_sensor_fps *fps)
 *
 * 获取摄像头输出帧率
 *
 * @param[in] num       对应sensor的标号
 * @param[in] fps       帧率属性
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @code
 * int ret = 0;
 * isp_sensor_fps fps;
 *
 * ret = isp_tuning_get_sensor_fps(fd, &fps);
 * if(ret){
 *     printf"isp_tuning_get_sensor_fps error !\n");
 *     return -1;
 * }
 * printf("fps:%f\n", (float)fps.num/fps.den);
 * @endcode
 *
 * @attention 在使用这个函数之前，必须保证ISP_EnableSensor 和 ISP_EnableTuning已被调用。
 * @attention 在使能帧通道开始传输数据之前必须先调用此函数获取摄像头默认帧率。
 */
int32_t isp_tuning_get_sensor_fps(int fd, isp_sensor_fps *fps);

/**
 * ISP抗闪频功能模式结构体。
 */
typedef enum {
    ISP_ANTIFLICKER_DISABLE_MODE,           /**< 不使能ISP抗闪频功能 */
    ISP_ANTIFLICKER_NORMAL_MODE,            /**< 使能ISP抗闪频功能的正常模式，即曝光最小值为第一个step，不能达到sensor的最小值 */
    ISP_ANTIFLICKER_AUTO_MODE,              /**< 使能ISP抗闪频功能的自动模式，最小曝光可以达到sensor曝光的最小值 */
    ISP_ANTIFLICKER_BUTT,                   /**< 用于判断参数的有效性，参数大小必须小于这个值 */
} ISPAntiflickerMode;

/**
 * ISP抗闪频属性参数结构体。
 */
typedef struct {
    ISPAntiflickerMode mode;    /**< ISP抗闪频功能模式选择 */
    uint8_t freq;               /**< 设置抗闪的工频 */
} ISPAntiflickerAttr;


/**
 * @fn int isp_tuning_get_anti_flicker_attr(int fd, enum ISPAntiflickerAttr *attr)
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
int isp_tuning_get_anti_flicker_attr(int fd, ISPAntiflickerAttr *attr);

/**
 * @fn int isp_tuning_set_anti_flicker_attr(int fd, enum ISPAntiflickerAttr attr)
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
int isp_tuning_set_anti_flicker_attr(int fd, ISPAntiflickerAttr *attr);

/**
 * gamma
 */
typedef enum {
        ISP_GAMMA_CURVE_DEFAULT,
        ISP_GAMMA_CURVE_SRGB,
        ISP_GAMMA_CURVE_REC709,
        ISP_GAMMA_CURVE_HDR,
        ISP_GAMMA_CURVE_USER,
        ISP_GAMMA_CURVE_BUTT,
} tisp_gamma_type_t;

typedef struct {
    tisp_gamma_type_t type;
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


 typedef struct {
    unsigned int hts;       /* sensor hts */
    unsigned int vts;       /* sensor vts */
    unsigned int fps;       /* sensor fps */
    unsigned int width;     /* sensor width */
    unsigned int height;    /* sensor height */
}isp_sensor_attr;
/**
* @fn int isp_tuning_get_sensor_attr(int fd, isp_sensor_attr *sensor_attr)
*
* 获取sensor_attr参数.
* @param[out] sensor_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_sensor_attr(int fd, isp_sensor_attr *sensor_attr);

/**
 * 权重信息
 */
typedef struct {
    unsigned char weight[15][15];    /**< 各区域权重信息 [0 ~ 8]*/
} isp_weight;

/**
* @fn int isp_tuning_get_awb_weight(int fd, isp_weight *weight)
*
* 获取weight参数.
* @param[out] weight参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_awb_weight(int fd, isp_weight *weight);
/**
* @fn int isp_tuning_set_awb_weight(int fd, isp_weight *weight)
*
* 获取weight参数.
* @param[out] sensor_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_awb_weight(int fd, isp_weight *weight);

/**
 * 白平衡增益属性
 */
typedef struct {
    uint32_t rgain;     /**< 白平衡R通道增益 */
    uint32_t bgain;     /**< 白平衡B通道增益 */
} awb_gain;

/**
 * 白平衡模式
 */
typedef enum {
    ISP_CORE_WB_MODE_AUTO = 0,          /**< 自动模式 */
    ISP_CORE_WB_MODE_MANUAL,            /**< 手动模式 */
    ISP_CORE_WB_MODE_DAY_LIGHT,         /**< 晴天 */
    ISP_CORE_WB_MODE_CLOUDY,            /**< 阴天 */
    ISP_CORE_WB_MODE_INCANDESCENT,                  /**< 白炽灯 */
    ISP_CORE_WB_MODE_FLOURESCENT,                   /**< 荧光灯 */
    ISP_CORE_WB_MODE_TWILIGHT,          /**< 黄昏 */
    ISP_CORE_WB_MODE_SHADE,             /**< 阴影 */
    ISP_CORE_WB_MODE_WARM_FLOURESCENT,              /**< 暖色荧光灯 */
    ISP_CORE_WB_MODE_COLORTEND,         /**< 自定义模式 */
} awb_mode;

/**
 * 白平衡自定义模式属性
 */
typedef struct {
    isp_tuning_ops_mode customEn;   /**< 白平衡自定义模式使能 */
    awb_gain gainH;            /**< 白平衡高色温通道增益偏移 */
    awb_gain gainM;            /**< 白平衡中色温通道增益偏移 */
    awb_gain gainL;            /**< 白平衡低色温通道增益偏移 */
    uint32_t ct_node[4];            /**< 白平衡通道增益偏移的节点 */
} awb_custom_mode_attr;

/**
 * 白平衡属性
 */
typedef struct isp_core_wb_attr{
    awb_mode mode;                      /**< 白平衡模式 */
    awb_gain gain_val;            /**< 白平衡通道增益，手动模式时有效 */
    isp_tuning_ops_mode awb_frz;            /**< 白平衡frzzen 使能*/
    unsigned int ct;                        /**< 白平衡当前色温值 */
    awb_custom_mode_attr custom;        /**< 白平衡自定义模式属性 */
    isp_tuning_ops_mode awb_start_en;       /**< 白平衡收敛起始点使能 */
    awb_gain awb_start;                 /**< 白平衡收敛起始点 */
} isp_awb_attr;

/**
* @fn int isp_tuning_get_awb_attr(int fd, isp_awb_attr *awb_attr)
*
* 获取awb_attr参数.
* @param[out] awb_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_awb_attr(int fd, isp_awb_attr *awb_attr);
/**
* @fn int isp_tuning_set_awb_attr(int fd, isp_awb_attr *awb_attr)
*
* 获取awb_attr参数.
* @param[out] awb_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_awb_attr(int fd, isp_awb_attr *awb_attr);

/**
 * 各区域统计信息
 */
typedef struct {
    uint32_t statis[15][15];    /**< 各区域统计信息*/
}  __attribute__((packed, aligned(1))) isp_statis_zone;

/**
 * AWB统计信息
 */
typedef struct {
    isp_statis_zone awb_r;    /**< AWB R通道统计值 */
    isp_statis_zone awb_g;    /**< AWB G通道统计值 */
    isp_statis_zone awb_b;    /**< AWB B通道统计值 */
} isp_awb_statis_info;

/**
* @fn int isp_tuning_get_awb_statis_zone(int fd, isp_awb_statis_info *awb_statis_info)
*
* 获取awb_statis_info参数.
* @param[out] awb_statis_info参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_awb_statis_zone(int fd, isp_awb_statis_info *awb_statis_info);

/**
 * AWB 全局统计信息
 */
typedef struct {
    awb_gain statis_weight_gain;    /**< 白平衡全局加权统计值 */
    awb_gain statis_gol_gain;       /**< 白平衡全局统计值 */
} isp_awb_statics_global;
/**
* @fn int isp_tuning_get_awb_global_statis(int fd, isp_awb_statics_global *awb_statis_info)
*
* 获取awb_statis_info参数.
* @param[out] awb_statis_info参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_awb_global_statis(int fd, isp_awb_statics_global *awb_statis_info);

/**
 * AE场景模式状态
 */
typedef enum {
    ISP_AE_SCENCE_AUTO,             /**< 自动模式 */
    ISP_AE_SCENCE_DISABLE,          /**< 关闭此场景模式 */
    TISP_AE_SCENCE_ROI_ENABLE,      /**< ROI 使能此场景模式 */
    TISP_AE_SCENCE_GLOBAL_ENABLE,   /**< GLOBAL 使能此场景模式 */
    ISP_AE_SCENCE_BUTT,             /**< 用于判断参数的有效性，参数大小必须小于这个值 */
} ae_scence_mode;

/**
 * AE场景模式属性
 */
typedef struct {
    ae_scence_mode AeHLCEn;            /**< AE 强光抑制功能使能 */
    unsigned char AeHLCStrength;           /**< AE 强光抑制强度（0 ~ 10）*/
    ae_scence_mode AeBLCEn;            /**< AE 背光补偿功能使能 */
    unsigned char AeBLCStrength;           /**< AE 背光补偿强度（0 ~ 10） */
    ae_scence_mode AeTargetCompEn;     /**< AE 目标亮度补偿使能 */
    uint32_t AeTargetComp;                 /**< AE 目标亮度调节强度（0 ~ 255，小于128变暗，大于128变亮） */
    ae_scence_mode AeStartEn;          /**< AE 起始点功能使能 */
    uint32_t AeStartEv;                    /**< AE 起始点EV值 */

    uint32_t luma;                         /**< AE Luma值 */
    uint32_t luma_scence;                  /**< AE 场景Luma值 */
} isp_ae_scence_attr;

/**
* @fn int isp_tuning_get_isp_ae_scence_attr(int fd, isp_ae_scence_attr *ae_scence_attr)
*
* 获取ae_scence_attr参数.
* @param[out] ae_scence_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_isp_ae_scence_attr(int fd, isp_ae_scence_attr *ae_scence_attr);

/**
* @fn int isp_tuning_set_isp_ae_scence_attr(int fd, isp_ae_scence_attr *ae_scence_attr)
*
* 获取ae_scence_attr参数.
* @param[out] ae_scence_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_isp_ae_scence_attr(int fd, isp_ae_scence_attr *ae_scence_attr);

/**
 * AE曝光时间单位
 */
typedef enum {
    ISP_CORE_EXPR_UNIT_LINE,    /**< 单位为曝光行 */
    ISP_CORE_EXPR_UNIT_US,      /**< 单位为微秒 */
} IMPISPAEIntegrationTimeUnit;

/**
 * AE曝光信息
 */
typedef struct {
    IMPISPAEIntegrationTimeUnit AeIntegrationTimeUnit;  /**< AE曝光时间单位 */
    isp_tuning_ops_mode AeMode;                         /**< AE Freezen使能 */
    isp_tuning_ops_mode AeIntegrationTimeMode;          /**< AE曝光手动模式使能 */
    isp_tuning_ops_mode AeAGainManualMode;              /**< AE Sensor 模拟增益手动模式使能 */
    isp_tuning_ops_mode AeDGainManualMode;              /**< AE Sensor数字增益手动模式使能 */
    isp_tuning_ops_mode AeIspDGainManualMode;        /**< AE ISP 数字增益手动模式使能 */
    uint32_t AeIntegrationTime;                         /**< AE手动模式下的曝光值 */
    uint32_t AeAGain;                                   /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
    uint32_t AeDGain;                                   /**< AE Sensor数字增益值，单位是倍数 x 1024 */
    uint32_t AeIspDGain;                                /**< AE ISP 数字增益值，单位倍数 x 1024*/

    isp_tuning_ops_mode AeMinIntegrationTimeMode;       /**< AE最小曝光使能位(预留) */
    isp_tuning_ops_mode AeMinAGainMode;                 /**< AE最小模拟增益使能位 */
    isp_tuning_ops_mode AeMinDgainMode;                 /**< AE最小数字增益使能位(预留) */
    isp_tuning_ops_mode AeMinIspDGainMode;              /**< AE最小ISP数字增益使能位(预留) */
    isp_tuning_ops_mode AeMaxIntegrationTimeMode;       /**< AE最大曝光使能位 */
    isp_tuning_ops_mode AeMaxAGainMode;                 /**< AE最大sensor模拟增益使能位 */
    isp_tuning_ops_mode AeMaxDgainMode;                 /**< AE最大sensor数字增益使能位 */
    isp_tuning_ops_mode AeMaxIspDGainMode;              /**< AE最大ISP数字增益使能位 */
    uint32_t AeMinIntegrationTime;                      /**< AE最小曝光时间 */
    uint32_t AeMinAGain;                                /**< AE最小sensor模拟增益，单位是倍数 x 1024 */
    uint32_t AeMinDgain;                                /**< AE最小sensor数字增益，单位是倍数 x 1024 */
    uint32_t AeMinIspDGain;                             /**< AE最小ISP数字增益，单位是倍数 x 1024 */
    uint32_t AeMaxIntegrationTime;                      /**< AE最大曝光时间 */
    uint32_t AeMaxAGain;                                /**< AE最大sensor模拟增益，单位是倍数 x 1024 */
    uint32_t AeMaxDgain;                                /**< AE最大sensor数字增益，单位是倍数 x 1024 */
    uint32_t AeMaxIspDGain;                             /**< AE最大ISP数字增益，单位是倍数 x 1024 */

    /* WDR模式下短帧的AE 手动模式属性*/
    isp_tuning_ops_mode AeShortMode;                    /**< AE Freezen使能 */
    isp_tuning_ops_mode AeShortIntegrationTimeMode;     /**< AE曝光手动模式使能 */
    isp_tuning_ops_mode AeShortAGainManualMode;         /**< AE Sensor 模拟增益手动模式使能 */
    isp_tuning_ops_mode AeShortDGainManualMode;         /**< AE Sensor数字增益手动模式使能 */
    isp_tuning_ops_mode AeShortIspDGainManualMode;      /**< AE ISP 数字增益手动模式使能 */
    uint32_t AeShortIntegrationTime;                    /**< AE手动模式下的曝光值 */
    uint32_t AeShortAGain;                              /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
    uint32_t AeShortDGain;                              /**< AE Sensor数字增益值，单位是倍数 x 1024 */
    uint32_t AeShortIspDGain;                           /**< AE ISP 数字增益值，单位倍数 x 1024*/

    isp_tuning_ops_mode AeShortMinIntegrationTimeMode;  /**< AE最小曝光使能位(预留) */
    isp_tuning_ops_mode AeShortMinAGainMode;            /**< AE最小模拟增益使能位 */
    isp_tuning_ops_mode AeShortMinDgainMode;            /**< AE最小数字增益使能位(预留) */
    isp_tuning_ops_mode AeShortMinIspDGainMode;         /**< AE最小ISP数字增益使能位(预留) */
    isp_tuning_ops_mode AeShortMaxIntegrationTimeMode;  /**< AE最大曝光使能位 */
    isp_tuning_ops_mode AeShortMaxAGainMode;            /**< AE最大sensor模拟增益使能位 */
    isp_tuning_ops_mode AeShortMaxDgainMode;            /**< AE最大sensor数字增益使能位 */
    isp_tuning_ops_mode AeShortMaxIspDGainMode;         /**< AE最大ISP数字增益使能位 */
    uint32_t AeShortMinIntegrationTime;                 /**< AE最小曝光时间 */
    uint32_t AeShortMinAGain;                           /**< AE最小sensor模拟增益 */
    uint32_t AeShortMinDgain;                           /**< AE最小sensor数字增益 */
    uint32_t AeShortMinIspDGain;                        /**< AE最小ISP数字增益 */
    uint32_t AeShortMaxIntegrationTime;                 /**< AE最大曝光时间 */
    uint32_t AeShortMaxAGain;                           /**< AE最大sensor模拟增益 */
    uint32_t AeShortMaxDgain;                           /**< AE最大sensor数字增益 */
    uint32_t AeShortMaxIspDGain;                        /**< AE最大ISP数字增益 */

    uint32_t TotalGainDb;                               /**< AE total gain，单位为db(只读) */
    uint32_t TotalGainDbShort;                          /**< AE 短帧 total gain, 单位为db */
    uint64_t ExposureValue;                             /**< AE 曝光值，为integration time x again x dgain */
    uint32_t EVLog2;                                    /**< AE 曝光值，此值经过log运算 */
} isp_ae_expr_info;

/**
* @fn int isp_tuning_get_isp_ae_expr_info(int fd, isp_ae_expr_info *ae_expr_info)
*
* 获取ae_expr_info参数.
* @param[out] ae_expr_info参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_isp_ae_expr_info(int fd, isp_ae_expr_info *ae_expr_info);

/**
* @fn int isp_tuning_set_isp_ae_expr_info(int fd, isp_ae_expr_info *ae_expr_info)
*
* 获取ae_expr_info参数.
* @param[out] ae_expr_info参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_isp_ae_expr_info(int fd, isp_ae_expr_info *ae_expr_info);

/**
 * AE统计信息
 */
typedef struct {
    unsigned short ae_hist_5bin[5];     /**< AE统计直方图bin值 [0 ~ 65535]*/
    uint32_t ae_hist_256bin[256];       /**< AE统计直方图bin值, 为每个bin的实际pixel数量*/
    isp_statis_zone ae_statis;          /**< AE统计信息 */
}  __attribute__((packed, aligned(1))) isp_ae_statis_attr;

/**
* @fn int isp_tuning_get_isp_ae_statis_attr(int fd, isp_ae_statis_attr *ae_statis_attr)
*
* 获取ae_statis_attr参数.
* @param[out] ae_statis_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_isp_ae_statis_attr(int fd, isp_ae_statis_attr *ae_statis_attr);


/**
 * 权重信息
 */
typedef struct {
    unsigned char weight[15][15];    /**< 各区域权重信息 [0 ~ 8]*/
} isp_ae_weight;

/**
 * AE权重信息
 */
typedef struct {
    isp_tuning_ops_mode roi_enable;    /**< 感兴趣区域权重设置使能(预留) */
    isp_tuning_ops_mode weight_enable; /**< 全局权重设置使能 */
    isp_ae_weight ae_roi;              /**< 感兴趣区域权重值(0 ~ 8)(预留) */
    isp_ae_weight ae_weight;           /**< 全局权重值(0~ 8) */
} isp_weight_attr;

/**
* @fn int isp_tuning_get_isp_ae_weight(int fd, isp_weight_attr *weight_attr)
*
* 获取weight_attr参数.
* @param[out] weight_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_isp_ae_weight(int fd, isp_weight_attr *weight_attr);

/**
* @fn int isp_tuning_set_isp_ae_weight(int fd, isp_weight_attr *weight_attr)
*
* 获取weight_attr参数.
* @param[out] weight_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_isp_ae_weight(int fd, isp_weight_attr *weight_attr);

/**
 * 统计值直方图统计色域结构体
 */
typedef enum {
    IMP_ISP_HIST_ON_RAW,    /**< Raw域 */
    IMP_ISP_HIST_ON_YUV,    /**< YUV域 */
} isp_hist_domain;

/**
 * 统计范围结构体
 */
typedef struct {
    unsigned int start_h;   /**< 横向起始点，单位为pixel AF统计值横向起始点：[1 ~ width]，且取奇数 */
    unsigned int start_v;   /**< 纵向起始点，单位为pixel AF统计值垂直起始点 ：[3 ~ height]，且取奇数 */
    unsigned char node_h;   /**< 横向统计区域块数 [12 ~ 15]*/
    unsigned char node_v;   /**< 纵向统计区域块数 [12 ~ 15]*/
} isp_3a_statis_location;

/**
 * AE统计值属性结构体
 */
typedef struct {
    isp_tuning_ops_mode ae_sta_en;  /**< AE统计功能开关*/
    isp_3a_statis_location local;   /**< AE统计位置(预留) */
    isp_hist_domain hist_domain;    /**< AE统计色域(预留) */
    unsigned char histThresh[4];    /**< AE直方图的分段 */
} isp_ae_statis_config;

/**
 * AWB统计值属性结构体
 */
typedef enum {
    IMP_ISP_AWB_ORIGIN,     /**< 原始统计值 */
    IMP_ISP_AWB_LIMITED,    /**< 加限制条件后的统计值 */
} IMPISPAWBStatisMode;

/**
 * AWB统计值属性结构体
 */
typedef struct {
    isp_tuning_ops_mode awb_sta_en;      /**< AWB统计功能开关*/
    isp_3a_statis_location local;        /**< AWB统计范围 */
    IMPISPAWBStatisMode mode;            /**< AWB统计属性(预留) */
} IMPISPAWBStatisAttr;

/**
 * AF统计属性结构体
 */
typedef struct {
    isp_tuning_ops_mode af_sta_en;      /**< AF统计功能开关*/
    isp_3a_statis_location local;       /**< AF统计范围 */
    unsigned char af_metrics_shift;     /**< AF统计值缩小参数 默认是0，1代表缩小2倍*/
    unsigned short af_delta;            /**< AF统计低通滤波器的权重 [0 ~ 64]*/
    unsigned short af_theta;            /**< AF统计高通滤波器的权重 [0 ~ 64]*/
    unsigned short af_hilight_th;       /**< AF高亮点统计阈值 [0 ~ 255]*/
    unsigned short af_alpha_alt;        /**< AF统计低通滤波器的水平与垂直方向的权重 [0 ~ 64]*/
    unsigned short af_belta_alt;        /**< AF统计低通滤波器的水平与垂直方向的权重 [0 ~ 64]*/
} IMPISPAFStatisAttr;

/**
 * 统计信息属性结构体
 */
typedef struct {
    isp_ae_statis_config ae;    /**< AE 统计信息属性 */
    IMPISPAWBStatisAttr awb;    /**< AWB 统计信息属性 */
    IMPISPAFStatisAttr af;      /**< AF 统计信息属性 */
} isp_statis_config;


/**
* @fn int isp_tuning_get_isp_statis_config(int fd, isp_statis_config *statis_config)
*
* 获取statis_config参数.
* @param[out] statis_config参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_isp_statis_config(int fd, isp_statis_config *statis_config);

/**
* @fn int isp_tuning_set_isp_statis_config(int fd, isp_statis_config *statis_config)
*
* 获取statis_config参数.
* @param[out] statis_config参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_isp_statis_config(int fd, isp_statis_config *ae_statis_config);

/**
 * AE曝光表属性
 */
typedef struct {
    isp_tuning_ops_mode mode;
    uint32_t elist[16][5];    /**< 列顺序说明：预留 | 曝光时间(us) | again | dgain | 预留 */
} isp_ae_list_attr;

/**
* @fn int isp_tuning_get_isp_ae_list_attr(int fd, isp_ae_list_attr *ae_list_attr)
*
* 获取ae_list_attr参数.
* @param[out] ae_list_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_get_isp_ae_list_attr(int fd, isp_ae_list_attr *ae_list_attr);

/**
* @fn int isp_tuning_set_isp_ae_list_attr(int fd, isp_ae_list_attr *ae_list_attr)
*
* 获取ae_list_attr参数.
* @param[out] ae_list_attr参数
*
* @retval 0 成功
* @retval 非0 失败，返回错误码
*
* @attention 在使用这个函数之前，必须保证ISP出图。
*/
int isp_tuning_set_isp_ae_list_attr(int fd, isp_ae_list_attr *ae_list_attr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* __ISPTuning_x2580_H__ */
