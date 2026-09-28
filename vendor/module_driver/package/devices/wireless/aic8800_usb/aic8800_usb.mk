#-------------------------------------------------------
package_name = aic8800_usb
package_depends = utils
package_module_src = devices/wireless/aic8800_usb/
package_make_hook =
package_init_hook =
package_finalize_hook = aic8800_usb_finalize_hook
package_clean_hook =
#-------------------------------------------------------

aic8800_loadfw_init_file = output/aic8800_aload_fw.sh
aic8800_fdrv_init_file = output/aic8800_fdrv.sh

define aic8800_usb_finalize_hook
	$(Q)cp devices/wireless/aic8800_usb/aic_load_fw/aic8800_aload_fw.ko output/aic8800_aload_fw.ko
	$(Q)cp devices/wireless/aic8800_usb/aic8800_fdrv/aic8800_fdrv.ko output/aic8800_fdrv.ko
	$(Q)echo 'insmod aic8800_aload_fw.ko \' > $(aic8800_loadfw_init_file)
	$(Q)echo 'insmod aic8800_fdrv.ko \' > $(aic8800_fdrv_init_file)
	$(Q)echo 'gpio_wlan_power=$(MD_AIC8800_USB_WLAN_POWER) \' >> $(aic8800_loadfw_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_AIC8800_USB_WLAN_REG_ON)' >> $(aic8800_loadfw_init_file)
endef
