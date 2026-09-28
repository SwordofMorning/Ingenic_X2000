# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh 2> /dev/null)

include $(TOPDIR)/output/build/buildroot-config/auto.conf
include .config.in

include rootfs_config/tools/set_sh_utils.mk
include rootfs_config/tools/set_login_tty.mk
include rootfs_config/tools/set_adb_server.mk
include rootfs_config/tools/set_ubi.mk
include rootfs_config/tools/set_jffs2.mk
include rootfs_config/tools/set_yaffs2.mk
include rootfs_config/tools/set_mmc.mk
include rootfs_config/tools/set_sdcard.mk
include rootfs_config/tools/set_mass_storage.mk
include rootfs_config/tools/set_h264_server.mk
include rootfs_config/tools/set_usb_storage.mk
include rootfs_config/tools/set_display_logo.mk
include rootfs_config/tools/set_usb_mtp_server.mk
include rootfs_config/tools/set_asound_conf.mk
include rootfs_config/tools/sdcard_update.mk
include rootfs_config/tools/set_third_party_lvgl.mk
include rootfs_config/tools/set_wait_mcu_boot_animation.mk
include rootfs_config/tools/set_adc_set_voltage.mk
include rootfs_config/tools/set_third_party_awtk.mk
include rootfs_config/tools/set_ssh.mk

define SYSTEM_CONFIG_SET_DEV_TMPFS
	rootfs_config/tools/add_mount_dev_tmpfs.sh $(TARGET_DIR)/etc/inittab
endef

define SYSTEM_CONFIG_SET_RCS
	cp -vf rootfs_config/file/rcS $(TARGET_DIR)/etc/init.d/rcS
endef

define SYSTEM_CONFIG_SET_S10MDEV
	cp -vf rootfs_config/file/S10mdev $(TARGET_DIR)/etc/init.d/
endef

all:
	$(SYSTEM_CONFIG_SET_SH_UTILS)
	$(SYSTEM_CONFIG_SET_DEV_TMPFS)
	$(SYSTEM_CONFIG_SET_LOGIN)
	$(SYSTEM_CONFIG_SET_S10MDEV)
	$(SYSTEM_CONFIG_SET_RCS)
	$(SYSTEM_CONFIG_SET_ADB)
	$(SYSTEM_CONFIG_SET_UBI)
	$(SYSTEM_CONFIG_SET_JFFS2)
	$(SYSTEM_CONFIG_SET_YAFFS2)
	$(SYSTEM_CONFIG_SET_MMC)
	$(SYSTEM_CONFIG_SET_SDCARD)
	$(SYSTEM_CONFIG_SET_MASS_STORAGE)
	$(SYSTEM_CONFIG_SET_H264)
	$(SYSTEM_CONFIG_USB_STORAGE)
	$(SYSTEM_CONFIG_SET_DISPLAY_LOGO)
	$(SYSTEM_CONFIG_SET_USB_MTP)
	$(SYSTEM_CONFIG_SET_ASOUND_CONF)
	$(SYSTEM_CONFIG_SDCARD_UPDATE)
	$(SYSTEM_CONFIG_SET_THIRD_PARTY_LVGL)
	$(SYSTEM_CONFIG_SET_BOOT_ANIMATION)
	$(SYSTEM_CONFIG_SET_ADC_SET_VOLTAGE)
	$(SYSTEM_CONFIG_SET_THIRD_PARTY_AWTK)
	$(SYSTEM_CONFIG_SET_SSH)
