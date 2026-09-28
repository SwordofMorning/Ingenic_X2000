#-------------------------------------------------------
package_name = aic8800d80
package_depends = utils soc_msc
package_module_src = devices/wireless/aic8800D80/
package_make_hook =
package_init_hook =
package_finalize_hook = aic8800d80_finalize_hook
package_clean_hook =
#-------------------------------------------------------

aic8800d80_init_file = output/aic8800_bsp.sh
aic8800_fdrv_init_file = output/aic8800_fdrv.sh
aic8800_btlpm_init_file = output/aic8800_btlpm.sh

define aic8800d80_finalize_hook
	$(Q)cp devices/wireless/aic8800D80/aic8800_bsp/aic8800_bsp.ko output/aic8800_bsp.ko
	$(Q)cp devices/wireless/aic8800D80/aic8800_fdrv/aic8800_fdrv.ko output/aic8800_fdrv.ko
	$(Q)cp devices/wireless/aic8800D80/aic8800_btlpm/aic8800_btlpm.ko output/aic8800_btlpm.ko
	$(Q)echo 'insmod aic8800_bsp.ko \' > $(aic8800d80_init_file)
	$(Q)echo 'insmod aic8800_fdrv.ko \' > $(aic8800_fdrv_init_file)
	$(Q)echo 'insmod aic8800_btlpm.ko \' > $(aic8800_btlpm_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_AIC8800D80_MMC_NUM) \' >> $(aic8800d80_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_AIC8800D80_WLAN_REG_ON) \' >> $(aic8800d80_init_file)
	$(Q)echo 'gpio_wlan_wake_host=$(MD_AIC8800D80_WLAN_WAKE_HOST) \' >> $(aic8800d80_init_file)
endef
