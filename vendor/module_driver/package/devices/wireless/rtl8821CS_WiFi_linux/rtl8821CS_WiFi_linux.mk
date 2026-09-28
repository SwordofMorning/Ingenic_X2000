#-------------------------------------------------------
package_name = rtl8821cs
package_depends = utils soc_msc
package_module_src = devices/wireless/rtl8821CS_WiFi_linux/
package_make_hook =
package_init_hook =
package_finalize_hook = rtl8821cs_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rtl8821cs_init_file = output/rtl8821cs.sh

define rtl8821cs_finalize_hook
	$(Q)cp devices/wireless/rtl8821CS_WiFi_linux/8821cs.ko output/rtl8821cs.ko
	$(Q)echo 'insmod rtl8821cs.ko \' > $(rtl8821cs_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_RTL8821CS_MMC_NUM) \' >> $(rtl8821cs_init_file)
	$(Q)echo 'gpio_wl_dis_n=$(MD_RTL8821CS_WLAN_DIS_N) \' >> $(rtl8821cs_init_file)
	$(Q)echo 'gpio_wl_rst=$(MD_RTL8821CS_WLAN_RESET) \' >> $(rtl8821cs_init_file)
	$(Q)echo 'gpio_bt_dis_n=$(MD_RTL8821CS_BT_DIS_N) \' >> $(rtl8821cs_init_file)
	$(Q)echo 'gpio_bt_wifi_power_en=$(MD_RTL8821CS_BT_WIFI_POWER_EN) \' >> $(rtl8821cs_init_file)
	$(Q)echo 'bt_wifi_power_valid_level=$(MD_RTL8821CS_BT_WIFI_POWER_VALID_LEVEL) \' >> $(rtl8821cs_init_file)
	$(Q)echo '' >> $(rtl8821cs_init_file)
endef
