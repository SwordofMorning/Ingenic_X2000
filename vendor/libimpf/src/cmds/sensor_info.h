#ifndef __SENSOR_INFO_H__
#define __SENSOR_INFO_H__



#define SENSOR_OV9281
#define SENSOR_CUBS_TYPE                TX_SENSOR_CONTROL_INTERFACE_I2C
#define SENSOR_I2C_ID                   0


#if defined SENSOR_OV2735
#define SENSOR_NAME                     "ov2735"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x3c
#define SENSOR_WIDTH                    1920
#define SENSOR_HEIGHT                   1080
#elif defined SENSOR_OV9281
#define SENSOR_NAME                     "ov9281"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
//#define OUTPUT_PIX_FMT                  PIX_FMT_RAW
#define SENSOR_I2C_ADDR                 0x60//0x10 注意ov9281 sensor地址有两个,一个不行尝试另外一个
#define SENSOR_WIDTH                    640
#define SENSOR_HEIGHT                   480
#elif defined SENSOR_OV5693
#define SENSOR_NAME                     "ov5693"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
//#define OUTPUT_PIX_FMT                  PIX_FMT_RAW
#define SENSOR_I2C_ADDR                 0x10//0x36 注意ov5693 sensor地址有两个,一个不行尝试另外一个
#define SENSOR_WIDTH                    1280
#define SENSOR_HEIGHT                   960
#elif defined SENSOR_SC132
#define SENSOR_NAME                     "sc132gs"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
//#define OUTPUT_PIX_FMT                  PIX_FMT_RAW
#define SENSOR_I2C_ADDR                 0x30
#define SENSOR_WIDTH                    1072
#define SENSOR_HEIGHT                   1280
#elif defined SENSOR_SC031
#define SENSOR_NAME                     "sc031"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
//#define OUTPUT_PIX_FMT                  PIX_FMT_RAW
#define SENSOR_I2C_ADDR                 0x30
#define SENSOR_WIDTH                    640
#define SENSOR_HEIGHT                   480

#elif defined SENSOR_OV9732
#define SENSOR_NAME                     "ov9732"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
//#define OUTPUT_PIX_FMT                  PIX_FMT_RAW
#define SENSOR_I2C_ADDR                 0x10//0x36 注意ov9732 sensor地址有两个,一个不行尝试另外一个
#define SENSOR_WIDTH                    1280
#define SENSOR_HEIGHT                   720
#elif defined SENSOR_GC0328
#define SENSOR_NAME                     "gc0328"
#define SENSOR_I2C_ADDR                 0x21
#define OUTPUT_PIX_FMT                  PIX_FMT_YUYV422
#define SENSOR_WIDTH                    640
#define SENSOR_HEIGHT                   480
#elif defined SENSOR_GC1034
#define SENSOR_NAME                     "gc1034"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x21
#define SENSOR_WIDTH                    1280
#define SENSOR_HEIGHT                   720
#elif defined SENSOR_GC2375A
#define SENSOR_NAME                     "gc2375a"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x37
#define SENSOR_WIDTH                    1600
#define SENSOR_HEIGHT                   1200
#elif defined SENSOR_BF3005
#define SENSOR_NAME                     "bf3005"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x6e
#define SENSOR_WIDTH                    640
#define SENSOR_HEIGHT                   480
#elif defined SENSOR_SP9250
#define SENSOR_NAME                     "sp9250"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x3c
#define SENSOR_WIDTH                    1600
#define SENSOR_HEIGHT                   1200
#elif defined SENSOR_SC2235
#define SENSOR_NAME                     "sc2235"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x30
#define SENSOR_WIDTH                    640
#define SENSOR_HEIGHT                   1072
#elif defined SENSOR_AR0230
#define SENSOR_NAME                     "ar0230"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x10
#define SENSOR_WIDTH                    1920
#define SENSOR_HEIGHT                   1080
#elif defined SENSOR_AR0522
#define SENSOR_NAME                     "ar0522"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x36
#define SENSOR_WIDTH                    2560
#define SENSOR_HEIGHT                   1080
#elif defined SENSOR_IMX307
#define SENSOR_NAME                     "imx307"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x1a
#define SENSOR_WIDTH                    1920
#define SENSOR_HEIGHT                   1080
#elif defined SENSOR_JXF23
#define SENSOR_NAME                     "jxf23"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x40
#define SENSOR_WIDTH                    640
#define SENSOR_HEIGHT                   1072
#elif defined SENSOR_S5K6A1
#define SENSOR_NAME                     "s5k6a1"
#define OUTPUT_PIX_FMT                  PIX_FMT_NV12
#define SENSOR_I2C_ADDR                 0x10
#define SENSOR_WIDTH                    1280
#define SENSOR_HEIGHT                   1024
#endif


#define SENSOR_WIDTH_SECOND             480
#define SENSOR_HEIGHT_SECOND            800

#define SENSOR_FPS_NUM                  25
#define SENSOR_FPS_DEN                  1



#endif
