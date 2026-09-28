/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for CPM
 *
 */

#ifndef __X2580_CAMERA_CPM_H__
#define __X2580_CAMERA_CPM_H__

#include <common.h>
#include <bit_field.h>

#define CPM_IOBASE                      0x10000000

/*
 * ISP
 */

#define CPM_SRBC                        (0xC4)

#define CPM_SRBC_ISP_SOFT_RESET         22, 22
#define CPM_SRBC_ISP_STOP_TRANSFER      21, 21
#define CPM_SRBC_ISP_STOP_ACK           20, 20


#define CAM_CPM_ADDR(reg)               ((volatile unsigned long *)CKSEG1ADDR(CPM_IOBASE + reg))

static inline void cpm_write_reg(unsigned int reg, unsigned int val)
{
    *CAM_CPM_ADDR(reg) = val;
}

static inline unsigned int cpm_read_reg(unsigned int reg)
{
    return *CAM_CPM_ADDR(reg);
}

static inline void cpm_set_bit(unsigned int reg, unsigned int start, unsigned int end, unsigned int val)
{
    set_bit_field(CAM_CPM_ADDR(reg), start, end, val);
}

static inline unsigned int cpm_get_bit(unsigned int reg, unsigned int start, unsigned int end)
{
    return get_bit_field(CAM_CPM_ADDR(reg), start, end);
}

#endif /* __X2580_CAMERA_CPM_H__ */
