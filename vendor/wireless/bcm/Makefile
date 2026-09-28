
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

all:
	@echo "fix make warning" > /dev/null

clean:
	@echo "fix make warning" > /dev/null

wifi_firmware-$(MD_WIFI_FIRMWARE_43438) = firmware/wifi_bcm/43438
wifi_firmware-$(MD_WIFI_FIRMWARE_43438_INFINEON) = firmware/wifi_bcm/43438_infineon
wifi_firmware-$(MD_WIFI_FIRMWARE_AP6212) = firmware/wifi_bcm/ap6212
wifi_firmware-$(MD_WIFI_FIRMWARE_43456C5) = firmware/wifi_bcm/43456c5
wifi_firmware-$(MD_WIFI_FIRMWARE_43455C0) = firmware/wifi_bcm/43455c0
wifi_firmware-$(MD_WIFI_FIRMWARE_43362) = firmware/wifi_bcm/43362

bt_firmware-$(MD_BT_FIRMWARE_43438) = /lib/firmware/bt_bcm/BCM4343A1_001.002.009.0077.0393.hcd
bt_firmware-$(MD_BT_FIRMWARE_43438_INFINEON) = /lib/firmware/bt_bcm/BCM4343A1_001.002.009.0122.0482.hcd
bt_firmware-$(MD_BT_FIRMWARE_AP6212) = /lib/firmware/bt_bcm/BCM4343A1_001.002.009.1010.1030.hcd
bt_firmware-$(MD_BT_FIRMWARE_43455C0) = /lib/firmware/bt_bcm/BCM4345C0_003.001.025.0140.0232.hcd
bt_firmware-$(MD_BT_FIRMWARE_43456C5) = /lib/firmware/bt_bcm/BCM4345C5_003.006.006.0058.0135.hcd

ifeq ($(MD_BT_BSA_BOOT_UP),y)
define INSTALL_BSA_BOOT_UP
	$(Q)cp bin/S41bt_bsa_download_firmware $(FS_TARGET_DIR)/etc/init.d/
endef
else
define INSTALL_BSA_BOOT_UP
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S41bt_bsa_download_firmware
endef
endif

ifeq ($(MD_BT_BSA_XBURST2),y)
define INSTALL_BSA_SERVER
	$(Q)cp -r bin/xburst2_bsa/bsa_server $(FS_TARGET_DIR)/usr/bin/
endef
else
define INSTALL_BSA_SERVER
	$(Q)cp -r bin/xburst1_bsa/bsa_server $(FS_TARGET_DIR)/usr/bin/
endef
endif

ifeq ($(APP_wireless_bcm),y)
define INSTALL_BSA_PACKAGE
	$(Q)cp bin/bt_enable_bsa.sh $(FS_TARGET_DIR)/usr/bin/
	$(INSTALL_BSA_BOOT_UP)
	$(INSTALL_BSA_SERVER)
	$(Q)echo "  installed bt_enable_bsa.sh bsa_server S41bt_bsa_download_firmware"
endef
endif

ifeq ($(MD_WIFI_BCM_BOOT_UP),y)
define INSTALL_BCM_BOOT_UP
	$(Q)cp bin/S44wifi_bcm_up $(FS_TARGET_DIR)/etc/init.d/
endef
else
define INSTALL_BCM_BOOT_UP
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S44wifi_bcm_up
endef
endif

ifeq ($(APP_wireless_bcm),y)
define INSTALL_BCM_PACKAGE
	$(INSTALL_BCM_BOOT_UP)
endef
endif

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)rm -rf $(FS_TARGET_DIR)/lib/firmware/wifi_bcm
	$(Q)rm -rf $(FS_TARGET_DIR)/lib/firmware/bt_bcm
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/bsa_server
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/bt_enable_bsa.sh
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S41bt_bsa_download_firmware
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S44wifi_bcm_up
	$(Q)rm -f $(FS_TARGET_DIR)/etc/profile.d/blue_set_uart_path.sh
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/wifi_up.sh
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/wifi_down.sh
	$(Q)rm -f $(FS_TARGET_DIR)/usr/bin/get_burn_partition_macaddr.sh
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S43wifi_bcm_init_config
	$(Q)echo "  removed wifi_*.sh lib/firmware/wifi_bcm/ "
	$(Q)echo "  removed bt_enable_bsa.sh bsa_server S41bt_bsa_download_firmware lib/firmware/bt_bcm/ "

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)mkdir -p $(FS_TARGET_DIR)/lib/firmware/
	$(Q)rm -rf $(FS_TARGET_DIR)/lib/firmware/wifi_bcm
	$(Q)cp -r $(wifi_firmware-y) $(FS_TARGET_DIR)/lib/firmware/wifi_bcm
	$(Q)cp -r firmware/bt_bcm/ $(FS_TARGET_DIR)/lib/firmware/
	$(Q)cp bin/wifi_up.sh $(FS_TARGET_DIR)/usr/bin/
	$(Q)cp bin/wifi_down.sh $(FS_TARGET_DIR)/usr/bin/
	$(Q)cp bin/get_burn_partition_macaddr.sh $(FS_TARGET_DIR)/usr/bin/
	$(Q)cp bin/S43wifi_bcm_init_config $(FS_TARGET_DIR)/etc/init.d/
	$(Q)echo "  installed wifi_*.sh lib/firmware/wifi_bcm/ "
	$(INSTALL_BCM_PACKAGE)
	$(INSTALL_BSA_PACKAGE)
	$(Q)sed -i "s/BT_UART_DEV_PATH/$(shell echo $(MD_BT_UART_DEV_PATH) | sed "s/\\//\\\\\//g")/g" $(FS_TARGET_DIR)/usr/bin/bt_enable_bsa.sh
	$(Q)sed -i "s/BT_FIRMWARE_PATH/$(shell echo $(bt_firmware-y) | sed "s/\\//\\\\\//g")/g" $(FS_TARGET_DIR)/usr/bin/bt_enable_bsa.sh

.PHONY: all clean install
