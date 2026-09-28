/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * version log
 *
 */
#ifndef __VERSION_LOG_H__
#define __VERSION_LOG_H__


/*
 * tisp core version
 */
#define TISP_CORE_VERSION               "1.1.1"


/*
 * isp驱动版本号说明：
 * 格式：x.y.z
 *   x：当驱动有较大结构性变动时更新
 *   y：当添加新功能或删除已有功能时更新
 *   z：没有功能变化但是内部实现有更新时更新
 * 注：当大版本更新时，次一级版本清零
 *
 * 日志记录：
 *     当x.y有变化时记录日志
 * 日志格式：
 *     版本号：
 *         功能说明日志
 *
 */

#define DRIVER_VERSION                  "1.2.1"

/**
     ---------- camera驱动日志 ----------
     [Camera, ISP, VIC, mscaler]


1.0.0：
    基本功能完成

1.1.0：
     修复Bug: mscaler多通道裁剪/缩放

1.2.0：
     修改ISP输出的宽度为8字节对齐

1.2.1:
     [Camera]开关流程优化：vic初始化、data bus(mipi/dvp)初始化位置变更

1.2.2:
    修复ISP重复初始化, 重复开关流Bug
 */



#endif /* __VERSION_LOG_H__ */
