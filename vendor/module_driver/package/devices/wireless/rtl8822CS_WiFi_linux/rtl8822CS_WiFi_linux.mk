#-------------------------------------------------------
package_name = rtl8822cs
package_depends = utils soc_msc
package_module_src = devices/wireless/rtl8822CS_WiFi_linux/
package_make_hook =
package_init_hook =
package_finalize_hook = rtl8822cs_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rtl8822cs_init_file = output/rtl8822cs.sh

define rtl8822cs_finalize_hook
	$(Q)cp devices/wireless/rtl8822CS_WiFi_linux/rtl8822cs.ko output/rtl8822cs.ko
	$(Q)echo 'insmod rtl8822cs.ko \' > $(rtl8822cs_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_RTL8822CS_MMC_NUM) \' >> $(rtl8822cs_init_file)
	$(Q)echo 'gpio_wl_dis_n=$(MD_RTL8822CS_WLAN_DIS_N) \' >> $(rtl8822cs_init_file)
	$(Q)echo 'gpio_wl_rst=$(MD_RTL8822CS_WLAN_RESET) \' >> $(rtl8822cs_init_file)
	$(Q)echo 'gpio_bt_dis_n=$(MD_RTL8822CS_BT_DIS_N) \' >> $(rtl8822cs_init_file)
	$(Q)echo 'gpio_bt_wifi_power_en=$(MD_RTL8822CS_BT_WIFI_POWER_EN) \' >> $(rtl8822cs_init_file)
	$(Q)echo 'bt_wifi_power_valid_level=$(MD_RTL8822CS_BT_WIFI_POWER_VALID_LEVEL) \' >> $(rtl8822cs_init_file)
	$(Q)echo '' >> $(rtl8822cs_init_file)
endef
