
# ------------------------------------------------------------
# 编译/清除 lvgl apps
# ------------------------------------------------------------
include .config.in

CFLAGS += -Wall
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -O2
CFLAGS += -include config.h
ifeq ($(APP_lvgl_enable_backtrace),y)
CFLAGS += -g -rdynamic -fasynchronous-unwind-tables
else
LDFLAGS += -Wl,--gc-sections
endif

LDFLAGS += liblvgl.a
LDFLAGS += -lhardware2 -lutils2 -lm -pthread
LDFLAGS += -Wl,-rpath=./
ifdef APP_lvgl_use_jpeg_turbo
LDFLAGS += -ljpeg
endif

CFLAGS += -include config.h -I./
CFLAGS += -Iingenic/
CFLAGS += -DLV_CONF_INCLUDE_SIMPLE

sinclude .lvgl_cflags.mk

src-$(APP_lvgl_demo_widgets) += main_lv_demo_widgets.c
module_name := lv_demo_widgets
include tools/build_elf.mk

LDFLAGS-$(APP_lvgl_video_player) += -lmedia -lmedia_ffmpeg -lpthread
src-$(APP_lvgl_video_player) += main_lv_widgets_player.c
module_name := lv_widgets_player
include tools/build_elf.mk

LDFLAGS-$(APP_media_video_player) += -lmedia -lmedia_ffmpeg -lpthread
src-$(APP_media_video_player) += main_video_player.c
module_name := video_player
include tools/build_elf.mk

src-$(APP_lvgl_demo_ilock) += demo_ilock/main.c
src-$(APP_lvgl_demo_ilock) += demo_ilock/ui_desktop.c
src-$(APP_lvgl_demo_ilock) += demo_ilock/ui_app_camera.c
src-$(APP_lvgl_demo_ilock) += demo_ilock/ui_app_analog_clock.c
module_name := lv_demo_ilock
include tools/build_elf.mk


LDFLAGS-y += -lmedia -lmedia_ffmpeg -lpthread
src-$(APP_lvgl_demo_ilock2) += demo_ilock2/main.c
src-$(APP_lvgl_demo_ilock2) += demo_ilock2/ui_desktop.c
src-$(APP_lvgl_demo_ilock2) += demo_ilock2/ui_app_camera.c
src-$(APP_lvgl_demo_ilock2) += demo_ilock2/ui_app_player.c
src-$(APP_lvgl_demo_ilock2) += demo_ilock2/ui_app_setting.c
src-$(APP_lvgl_demo_ilock2) += demo_ilock2/ui_app_brightness.c
ifdef APP_lvgl_h264_view_client
src-$(APP_lvgl_demo_ilock2) += demo_ilock2/ui_app_monitor.c
endif
module_name := lv_demo_ilock2
include tools/build_elf.mk


LDFLAGS-y += -lmedia -lpthread
src-$(APP_lvgl_demo_ilock_tuya) += demo_ilock_tuya/main.c
src-$(APP_lvgl_demo_ilock_tuya) += demo_ilock_tuya/ui_desktop.c
src-$(APP_lvgl_demo_ilock_tuya) += demo_ilock_tuya/ui_app_camera.c
# src-$(APP_lvgl_demo_ilock_tuya) += demo_ilock_tuya/ui_app_player.c
src-$(APP_lvgl_demo_ilock_tuya) += demo_ilock_tuya/ui_app_setting.c
src-$(APP_lvgl_demo_ilock_tuya) += demo_ilock_tuya/ui_app_brightness.c
module_name := lv_demo_ilock_tuya
include tools/build_elf.mk

src-$(APP_lvgl_demo_ilock_txt) += demo_ilock_txt/main.c
src-$(APP_lvgl_demo_ilock_txt) += demo_ilock_txt/ui_desktop.c
module_name := lv_demo_ilock_txt
include tools/build_elf.mk

ifdef APP_lvgl_demo_ilock_tuya
define copy_demo_ilock_resource_tuya
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock_tuya
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/lv_demo_ilock_tuya
	$(Q)cp lv_demo_ilock_tuya $(FS_TARGET_DIR)/etc/lv_demo_ilock_tuya/
	$(Q)cp demo_ilock_tuya/res/ -r $(FS_TARGET_DIR)/etc/lv_demo_ilock_tuya/
endef
define clean_demo_ilock_resource_tuya
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock_tuya
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/lv_demo_ilock_tuya
endef
endif


ifdef APP_lvgl_demo_ilock2
define copy_demo_ilock_resource2
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock2
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/lv_demo_ilock2
	$(Q)cp lv_demo_ilock2 $(FS_TARGET_DIR)/etc/lv_demo_ilock2/
	$(Q)cp demo_ilock2/res/ -r $(FS_TARGET_DIR)/etc/lv_demo_ilock2/
endef
define clean_demo_ilock_resource2
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock2
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/lv_demo_ilock2
endef
endif



ifdef APP_lvgl_demo_ilock
define copy_demo_ilock_resource
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/lv_demo_ilock
	$(Q)cp lv_demo_ilock $(FS_TARGET_DIR)/etc/lv_demo_ilock/
	$(Q)cp demo_ilock/res/ -r $(FS_TARGET_DIR)/etc/lv_demo_ilock/
endef
define clean_demo_ilock_resource
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/lv_demo_ilock
endef
endif

ifdef APP_lvgl_demo_ilock_txt
define copy_demo_ilock_resource_txt
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock_txt
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/lv_demo_ilock_txt
	$(Q)cp lv_demo_ilock_txt $(FS_TARGET_DIR)/etc/lv_demo_ilock_txt/
	$(Q)cp demo_ilock_txt/res/ -r $(FS_TARGET_DIR)/etc/lv_demo_ilock_txt/
endef
define clean_demo_ilock_resource_txt
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/lv_demo_ilock_txt
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/lv_demo_ilock_txt
endef
endif

apps: $(module_targets)
	$(Q)echo "  $(all_modules)"

clean_apps:
	$(Q)rm -rf $(module_clean_files)
	$(clean_demo_ilock_resource)
	$(clean_demo_ilock_resource2)
	$(clean_demo_ilock_resource_tuya)
	$(clean_demo_ilock_resource_txt)

install:
ifdef all_modules
	$(Q)cp $(all_modules) $(FS_TARGET_DIR)/usr/bin/
endif
	@echo "  installed $(all_modules)"
	$(copy_demo_ilock_resource)
	$(copy_demo_ilock_resource2)
	$(copy_demo_ilock_resource_tuya)
	$(copy_demo_ilock_resource_txt)

clean_install:
ifdef all_modules
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(all_modules)))
endif
	@echo "  removed $(all_modules)"

.PHONY: apps apps_clean install clean_install $(all_modules)
