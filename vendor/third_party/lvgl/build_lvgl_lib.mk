# ------------------------------------------------------------
# 编译/清除 liblvgl.so
# ------------------------------------------------------------
include .config.in

CFLAGS += -Wall
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -O2
CFLAGS += -include config.h -I./
CFLAGS += -DLV_CONF_INCLUDE_SIMPLE
# CFLAGS-y += -fPIC
ifeq ($(APP_lvgl_enable_backtrace),y)
CFLAGS +=  -g -rdynamic -fasynchronous-unwind-tables
endif

ifeq ($(APP_lvgl_use_msa),y)
CFLAGS += -mmsa
endif

CFLAGS += -DFT2_BUILD_LIBRARY
CFLAGS += -DFT_CONFIG_MODULES_H=\<ingenic/ftmodule.h\>
CFLAGS += -DFT_CONFIG_OPTIONS_H=\<ingenic/ftoption.h\>
# CFLAGS += -DFT_CONFIG_MODULES_H=\<lvgl/src/libs/freetype/ftmodule.h\>
# CFLAGS += -DFT_CONFIG_OPTIONS_H=\<lvgl/src/libs/freetype/ftoption.h\>

# FreeType include path
CFLAGS += -Ifreetype/include/
CFLAGS += -Iingenic/

# FreeType C source file
FT_CSRCS += freetype/src/base/ftbase.c
FT_CSRCS += freetype/src/base/ftmm.c
FT_CSRCS += freetype/src/base/ftbitmap.c
FT_CSRCS += freetype/src/base/ftdebug.c
FT_CSRCS += freetype/src/base/ftglyph.c
FT_CSRCS += freetype/src/base/ftinit.c
FT_CSRCS += freetype/src/cache/ftcache.c
FT_CSRCS += freetype/src/gzip/ftgzip.c
FT_CSRCS += freetype/src/sfnt/sfnt.c
FT_CSRCS += freetype/src/smooth/smooth.c
FT_CSRCS += freetype/src/truetype/truetype.c


sinclude .lvgl_files.mk
sinclude .lvgl_cflags.mk

src-y += $(FT_CSRCS)
# src-y += lvgl_fb_display_v9.0.c
# src-y += lvgl_tp_input_v9.0.c
src-y += ingenic/lvgl_fb_display_v8.39.c
src-y += ingenic/lvgl_tp_input_v8.39.c
src-y += ingenic/lvgl_usleep_loop.c

src-y += ingenic/utils/ilv_time.c
src-y += ingenic/utils/ilv_utils.c

src-y += ingenic/style/ilv_style.c
src-y += ingenic/style/ilv_style_normal_views.c
src-y += ingenic/style/ilv_style_meter.c
src-y += ingenic/style/ilv_style_rotate_img.c
src-y += ingenic/style/ilv_style_text_doing.c
src-y += ingenic/style/ilv_style_analog_clock.c
src-y += ingenic/style/ilv_style_digital_clock.c

src-y += ingenic/parser/ilv_config.c
src-y += ingenic/parser/ilv_config_font.c
src-y += ingenic/parser/ilv_parser.c
src-y += ingenic/parser/parse_str.c
src-y += ingenic/parser/parse_style.c
src-y += ingenic/parser/ilv_parser_event.c

src-y += ingenic/lv_ftsystem.c
src-y += ingenic/signal_handler.c
src-$(APP_lvgl_use_jpeg_turbo) += ingenic/lv_libjpeg_turbo.c

src-y += ingenic/lv_ingenic_sw_blend.c
src-y += ingenic/view/ilv_color_picker.c
src-y += ingenic/view/lv_draw_sw_rotate.c
src-y += ingenic/view/ilv_rotate_img.c
src-y += ingenic/view/ilv_text_doing.c
src-y += ingenic/view/ilv_analog_clock.c
src-y += ingenic/view/ilv_digital_clock.c

src-$(APP_lvgl_h264_view_client) += ingenic/view/ilv_h264_view.c
src-$(APP_lvgl_h264_view_client) += ingenic/view/ingenic_h264_view_client.c

module_name = liblvgl.a
include tools/build_elf.mk

lvgl_dynamic_lib: $(module_targets)
	@echo "  $(all_modules)"

clean_lvgl_dynamic_lib:
	$(Q)rm -f $(module_clean_files)

install:
	$(Q)cp $(all_modules) $(FS_STAGING_DIR)/usr/lib/
	@echo "  installed $(all_modules)"

clean_install:
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir $(all_modules)))
	@echo "  removed $(all_modules)"

.PHONY: lvgl_dynamic_lib_clean lvgl_dynamic_lib install clean_install
