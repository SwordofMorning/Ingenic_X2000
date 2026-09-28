#-------------------------------------------------------
package_name = fcs950uab_wifi
package_depends = utils soc_msc fcs950uab_uwe5622_sdio
package_module_src = devices/wireless/fcs950uab/unisocwifi/
package_make_hook =
package_init_hook =
package_finalize_hook = fcs950uab_wifi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

fcs950uab_wifi_init_file = output/fcs950uab_wifi.sh

define fcs950uab_wifi_finalize_hook
	$(Q)cp devices/wireless/fcs950uab/unisocwifi/fcs950uab_wifi.ko output/
	$(Q)echo 'insmod fcs950uab_wifi.ko \' > $(fcs950uab_wifi_init_file)
endef