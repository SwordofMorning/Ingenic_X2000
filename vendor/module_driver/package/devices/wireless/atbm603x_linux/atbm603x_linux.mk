#-------------------------------------------------------
package_name = atbm_wifi.ko
package_depends = utils soc_msc
package_module_src = devices/wireless/atbm603x_linux/
package_make_hook =
package_init_hook =
package_finalize_hook = atbm603x_finalize_hook
package_clean_hook =
#-------------------------------------------------------

atbm603x_init_file = output/atbm_wifi.sh

define atbm603x_finalize_hook
	$(Q)cp devices/wireless/atbm603x_linux/hal_apollo/atbm_wifi.ko output/atbm_wifi.ko
	$(Q)echo 'insmod atbm_wifi.ko \' > $(atbm603x_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_ATBM603X_MMC_NUM) \' >> $(atbm603x_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_ATBM603X_WLAN_REG_ON) \' >> $(atbm603x_init_file)
	$(Q)echo 'gpio_wlan_wake_host=$(MD_ATBM603X_WLAN_WAKE_HOST) \' >> $(atbm603x_init_file)
endef
