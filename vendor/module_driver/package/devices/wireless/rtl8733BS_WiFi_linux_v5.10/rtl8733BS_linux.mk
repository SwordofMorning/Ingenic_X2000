#-------------------------------------------------------
package_name = rtl8733bs
package_depends = utils soc_msc
package_module_src = devices/wireless/rtl8733BS_WiFi_linux_v5.10/
package_make_hook =
package_init_hook =
package_finalize_hook = rtl8733bs_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rtl8733bs_init_file = output/rtl8733bs.sh

define rtl8733bs_finalize_hook
	$(Q)cp devices/wireless/rtl8733BS_WiFi_linux_v5.10/8733bs.ko output/rtl8733bs.ko
	$(Q)echo 'insmod rtl8733bs.ko \' > $(rtl8733bs_init_file)
	$(Q)echo 'wlan_mmc_num=$(MD_RTL8733BS_MMC_NUM) \' >> $(rtl8733bs_init_file)
	$(Q)echo 'gpio_wl_dis_n=$(MD_RTL8733BS_WLAN_DIS_N) \' >> $(rtl8733bs_init_file)
	$(Q)echo 'gpio_bt_dis_n=$(MD_RTL8733BS_BT_DIS_N) \' >> $(rtl8733bs_init_file)
	$(Q)echo 'gpio_bt_wifi_power_en=$(MD_RTL8733BS_BT_WIFI_POWER_EN) \' >> $(rtl8733bs_init_file)
	$(Q)echo 'bt_wifi_power_valid_level=$(MD_RTL8733BS_BT_WIFI_POWER_VALID_LEVEL) \' >> $(rtl8733bs_init_file)
	$(Q)echo '' >> $(rtl8733bs_init_file)
endef
