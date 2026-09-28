#-------------------------------------------------------
package_name = esp32_sdio
package_depends = utils
package_module_src = devices/wireless/esp32_linux/
package_make_hook =
package_init_hook =
package_finalize_hook = esp32_finalize_hook
package_clean_hook =
#-------------------------------------------------------

esp32_init_file = output/esp32_sdio.sh

define esp32_finalize_hook
	$(Q)cp devices/wireless/esp32_linux/esp32_sdio.ko output/esp32_sdio.ko
	$(Q)echo 'insmod esp32_sdio.ko \' > $(esp32_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_ESP32_MMC_NUM) \' >> $(esp32_init_file)
	$(Q)echo 'gpio_power_en=$(MD_ESP32_POWER_EN) \' >> $(esp32_init_file)
	$(Q)echo 'wifi_power_valid_level=$(MD_ESP32_WIFI_POWER_VALID_LEVEL) \' >> $(esp32_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_ESP32_WLAN_REG_ON) \' >> $(esp32_init_file)
	$(Q)echo 'gpio_download_mode=$(MD_ESP32_DOWNLOAD_MODE) \' >> $(esp32_init_file)
	$(Q)echo '' >> $(esp32_init_file)
endef