overlay_bootfs_path=../buildroot/buildroot_patch/system/overlay_bootfs

ifneq ($(APP_system_overlay_bootfs),)

define CMD_MAKE_OVERLAY_BOOTFS
	$(Q)make -C $(overlay_bootfs_path) config_in=$(config_in_file) --no-print-directory all
	$(Q)cp -v $(overlay_bootfs_path)/tmp_buildroot/output/images/rootfs.squashfs \
		 output/overlay_bootfs.squashfs
endef

define CMD_CLEAN_OVERLAY_BOOTFS
	$(Q)make -C $(overlay_bootfs_path) config_in=$(config_in_file) --no-print-directory clean
	$(Q)rm output/overlay_bootfs.squashfs
endef

endif
