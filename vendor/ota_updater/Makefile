
# 检查 .config.in include/config.h 是否有更新
make_sure_config_update:=$(shell tools/check_config.sh $(config_in) 2> /dev/null)

sinclude .config.in

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
MAKE_ARG = --no-print-directory
else
Q =
MAKE_ARG =
endif

OTA_ARG =
OTA_ARG += ota_version=$(APP_ota_updater_version)
OTA_ARG += block_size=$(App_ota_updater_block_size)
OTA_ARG += kernel_img=$(App_ota_updater_kernel_img_path)
OTA_ARG += rootfs_img=$(App_ota_updater_rootfs_img_path)
OTA_ARG += rtos_img=$(App_ota_updater_rtos_img_path)

all:
	@echo "fix make warning" > /dev/null

clean:
	@echo "fix make warning" > /dev/null

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)rm -f $(FS_TARGET_DIR)/etc/ota_info
	$(Q)echo "  removed etc/ota_info etc/ota_bin"

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)echo "ota_version=$(APP_ota_updater_version)" > $(FS_TARGET_DIR)/etc/ota_info
	$(Q)echo "ota_site=$(App_ota_updater_site)" >> $(FS_TARGET_DIR)/etc/ota_info
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/ota_img_data_provider.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/$(APP_ota_updater_storage_medium)/ota_update_kernel.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/$(APP_ota_updater_storage_medium)/ota_update_rootfs_squashfs.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/$(APP_ota_updater_storage_medium)/ota_update_rtos_bin.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/$(APP_ota_updater_storage_medium)/ota_utils.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/$(APP_ota_updater_storage_medium)/ota_local_method.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/local_ota_update.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/main_os_info_check.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/network_main_os_update_recovery.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/network_ota_update.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/network_recovery_update_main_os.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)cp device_tools/recovery_version_utils.sh $(FS_TARGET_DIR)/etc/ota_bin/
	$(Q)echo "  installed etc/ota_info etc/ota_bin"

ota_img:
	$(Q)./host_tools/mk_ota.sh $(OTA_ARG)

.PHONY: all clean install

