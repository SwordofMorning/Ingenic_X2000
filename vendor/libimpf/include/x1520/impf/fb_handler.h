/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic Linux plarform SDK project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef FB_MANAGER_H
#define FB_MANAGER_H


#include <stdint.h>


#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */




typedef struct {
    int (*init)(void);
    int (*deinit)(void);
    uint8_t* (*getFBMem)(void);                     /* 获取显示buffer地址 */
    int (*display)(void);                           /* 显示当前buffer内容，并切换下当前buffer地址（getFBMem地址）为下一个buffer，注：切换buffer会受限于驱动配置buffer个数 */
    int (*blank)(uint8_t blank);
    void (*dump)(void);

    uint32_t (*getScreenSize)(void);
    uint32_t (*getScreenHeight)(void);
    uint32_t (*getScreenWidth)(void);

    uint32_t (*getRedbitOffset)(void);
    uint32_t (*getRedbitLength)(void);

    uint32_t (*getGreenbitOffset)(void);
    uint32_t (*getGreenbitLength)(void);

    uint32_t (*getBluebitOffset)(void);
    uint32_t (*getBluebitLength)(void);

    uint32_t (*getAlphabitOffset)(void);
    uint32_t (*getAlphabitLength)(void);

    uint32_t (*getBitsPerPixel)(void);
    uint32_t (*getRowBytes)(void);
}IMPF_FBHandler;


IMPF_FBHandler* IMPF_GetFBHandler(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* FB_MANAGER_H */
