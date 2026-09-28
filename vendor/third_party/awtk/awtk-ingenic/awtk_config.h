
/**
 * File:   awtk_config.h
 * Author: AWTK Develop Team
 * Brief:  config
 *
 * Copyright (c) 2018 - 2023  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-09-12 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef AWTK_CONFIG_H
#define AWTK_CONFIG_H

#include "../config.h"

#ifdef APP_awtk_FB_DEVICE_FILENAME
#define FB_DEVICE_FILENAME   APP_awtk_FB_DEVICE_FILENAME
#endif

#ifdef APP_awtk_INPUT_DEVICE_NOSCAN
#define INPUT_DEVICE_NOSCAN   APP_awtk_INPUT_DEVICE_NOSCAN
#endif

#ifdef APP_awtk_INPUT_DEVICE_FILENAME
#define INPUT_DEVICE_FILENAME   APP_awtk_INPUT_DEVICE_FILENAME
#endif

#ifdef APP_awtk_USE_GUI_MAIN
#define USE_GUI_MAIN   APP_awtk_USE_GUI_MAIN
#endif

#ifdef APP_awtk_WITH_STB_IMAGE
#define WITH_STB_IMAGE   APP_awtk_WITH_STB_IMAGE
#endif

#ifdef APP_awtk_WITH_STB_FONT
#define WITH_STB_FONT   APP_awtk_WITH_STB_FONT
#endif

#ifdef APP_awtk_WITH_FT_FONT
#define WITH_FT_FONT   APP_awtk_WITH_FT_FONT
#endif

#ifdef APP_awtk_WITH_FS_RES
#define WITH_FS_RES   APP_awtk_WITH_FS_RES
#endif

#ifdef APP_awtk_WITH_UNICODE_BREAK
#define WITH_UNICODE_BREAK   APP_awtk_WITH_UNICODE_BREAK
#endif

#ifdef APP_awtk_WITH_BITMAP_BGRA
#define WITH_BITMAP_BGRA   APP_awtk_WITH_BITMAP_BGRA
#endif

#ifdef APP_awtk_WITH_BITMAP_BGR565
#define WITH_BITMAP_BGR565   APP_awtk_WITH_BITMAP_BGR565
#endif

#ifdef APP_awtk_WITH_BITMAP_RGB565
#define WITH_BITMAP_RGB565   APP_awtk_WITH_BITMAP_RGB565
#endif

#ifdef APP_awtk_WITH_NULL_IM
#define WITH_NULL_IM   APP_awtk_WITH_NULL_IM
#endif

#ifdef APP_awtk_HAS_STD_MALLOC
#define HAS_STD_MALLOC   APP_awtk_HAS_STD_MALLOC
#endif

#ifdef APP_awtk_HAS_PTHREAD
#define HAS_PTHREAD   APP_awtk_HAS_PTHREAD
#endif

#ifdef APP_awtk_HAS_STDIO
#define HAS_STDIO   APP_awtk_HAS_STDIO
#endif

#ifdef APP_awtk_HAS_FAST_MEMCPY
#define HAS_FAST_MEMCPY   APP_awtk_HAS_FAST_MEMCPY
#endif

#ifdef APP_awtk_WITH_NANOVG_AGGE
#define WITH_NANOVG_AGGE   APP_awtk_WITH_NANOVG_AGGE
#endif

#ifdef APP_awtk_WITH_NANOVG_AGG
#define WITH_NANOVG_AGG   APP_awtk_WITH_NANOVG_AGG
#endif

#ifdef APP_awtk_ENABLE_CURSOR
#define ENABLE_CURSOR   APP_awtk_ENABLE_CURSOR
#endif

#ifdef APP_awtk_WITH_VGCANVAS_CAIRO
#define WITH_VGCANVAS_CAIRO   APP_awtk_WITH_VGCANVAS_CAIRO
#endif

#ifdef APP_awtk_WITHOUT_WIDGET_ANIMATORS
#define WITHOUT_WIDGET_ANIMATORS   APP_awtk_WITHOUT_WIDGET_ANIMATORS
#endif

#ifdef APP_awtk_WITHOUT_WINDOW_ANIMATORS
#define WITHOUT_WINDOW_ANIMATORS   APP_awtk_WITHOUT_WINDOW_ANIMATORS
#endif

#ifdef APP_awtk_WITHOUT_DIALOG_HIGHLIGHTER
#define WITHOUT_DIALOG_HIGHLIGHTER   APP_awtk_WITHOUT_DIALOG_HIGHLIGHTER
#endif

#ifdef APP_awtk_WITHOUT_EXT_WIDGETS
#define WITHOUT_EXT_WIDGETS   APP_awtk_WITHOUT_EXT_WIDGETS
#endif

#ifdef APP_awtk_WITH_WIDGET_TYPE_CHECK
#define WITH_WIDGET_TYPE_CHECK   APP_awtk_WITH_WIDGET_TYPE_CHECK
#endif

#ifdef APP_awtk_WITH_WCSXXX
#define WITH_WCSXXX   APP_awtk_WITH_WCSXXX
#endif

#ifdef APP_awtk_WITH_3RD_NANOVG
#define WITH_3RD_NANOVG   APP_awtk_WITH_3RD_NANOVG
#endif

#ifdef APP_awtk_VGCANVAS_CAIRO
#define VGCANVAS_CAIRO   APP_awtk_VGCANVAS_CAIRO
#endif

#ifdef APP_awtk_NANOVG_BACKEND_AGG
#define NANOVG_BACKEND_AGG   APP_awtk_NANOVG_BACKEND_AGG
#endif

#ifdef APP_awtk_NANOVG_BACKEND_AGGE
#define NANOVG_BACKEND_AGGE   APP_awtk_NANOVG_BACKEND_AGGE
#endif

#ifdef APP_awtk_WITH_G2D
#define WITH_G2D   APP_awtk_WITH_G2D
#endif

#endif /*AWTK_CONFIG_H*/
