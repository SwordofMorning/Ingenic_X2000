#-------------------------------------------------------
package_name = fcs950uab_uwe5622_sdio
package_depends = utils soc_msc
package_module_src = devices/wireless/fcs950uab/unisocwcn/
package_make_hook =
package_init_hook =
package_finalize_hook = fcs950uab_uwe5622_sdio_finalize_hook
package_clean_hook =
#-------------------------------------------------------

fcs950uab_uwe5622_sdio_init_file = output/fcs950uab_uwe5622_sdio.sh

define fcs950uab_uwe5622_sdio_finalize_hook
	$(Q)cp devices/wireless/fcs950uab/unisocwcn/fcs950uab_uwe5622_sdio.ko output/
	$(Q)echo 'insmod fcs950uab_uwe5622_sdio.ko \' > $(fcs950uab_uwe5622_sdio_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_FCS950UAB_MMC_NUM) \' >> $(fcs950uab_uwe5622_sdio_init_file)
	$(Q)echo 'bt_wifi_power_valid_level=$(MD_FCS950UAB_BT_WIFI_POWER_VALID_LEVEL) \' >> $(fcs950uab_uwe5622_sdio_init_file)
	$(Q)echo 'gpio_bt_wifi_power=$(MD_FCS950UAB_BT_WIFI_POWER) \' >> $(fcs950uab_uwe5622_sdio_init_file)
	$(Q)echo 'gpio_wlan_reg_on=$(MD_FCS950UAB_WLAN_REG_ON) \' >> $(fcs950uab_uwe5622_sdio_init_file)
	$(Q)echo 'gpio_wlan_wake_host=$(MD_FCS950UAB_WLAN_WAKE_HOST) \' >> $(fcs950uab_uwe5622_sdio_init_file)
	$(Q)echo 'gpio_ap_irq=$(MD_FCS950UAB_AP_IRQ) \' >> $(fcs950uab_uwe5622_sdio_init_file)
endef