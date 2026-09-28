/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Mulitiple Channel Scaler
 *
 */


#ifndef __X2580_MSCALER_H__
#define __X2580_MSCALER_H__

#include "camera_isp.h"
#include "mscaler_regs.h"
#include "camera_sensor.h"

/*
 * MSCALER最大支持3个ch输出，每个ch支持不同的缩放等级.
 */
#define MSCALER_MAX_CH                  3

#define MSCALER_INPUT_MAX_WIDTH         2048
#define MSCALER_INPUT_MAX_HEIGHT        2048

#define MSCALER_OUTPUT_MIN_WIDTH        8
#define MSCALER_OUTPUT_MIN_HEIGHT       8
#define MSCALER_OUTPUT_MAX_WIDTH        4095
#define MSCALER_OUTPUT_MAX_HEIGHT       4095

/*
 * open uv rsmp, mscaler channel output resulotion will be limited
 * default:disable uv rsmp, mscaler channel output resulotion unlimited
 */
#define MSCALER_OUTPUT0_MAX_WIDTH       MSCALER_OUTPUT_MAX_WIDTH
#define MSCALER_OUTPUT0_MAX_HEIGHT      MSCALER_OUTPUT_MAX_WIDTH

#define MSCALER_OUTPUT1_MAX_WIDTH       MSCALER_OUTPUT_MAX_WIDTH
#define MSCALER_OUTPUT1_MAX_HEIGHT      MSCALER_OUTPUT_MAX_WIDTH

#define MSCALER_OUTPUT2_MAX_WIDTH       MSCALER_OUTPUT_MAX_WIDTH
#define MSCALER_OUTPUT2_MAX_HEIGHT      MSCALER_OUTPUT_MAX_WIDTH


int mscaler_interrupt_service_routine(int index, unsigned int status, unsigned int statusBack);

int mscaler_register_sensor(struct sensor_attr *sensor);
void mscaler_unregister_sensor(struct sensor_attr *sensor);

int jz_mscaler_drv_init(int index);
void jz_mscaler_drv_deinit(int index);


#endif /* __X2580_MSCALER_H__ */