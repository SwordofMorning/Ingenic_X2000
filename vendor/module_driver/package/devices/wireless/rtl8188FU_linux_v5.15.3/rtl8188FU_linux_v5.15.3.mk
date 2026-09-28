#-------------------------------------------------------
package_name = rtl8188fu
package_depends = utils
package_module_src = devices/wireless/rtl8188FU_linux_v5.15.3/
package_make_hook =
package_init_hook =
package_finalize_hook = rtl8188fu_finalize_hook
package_clean_hook =
#-------------------------------------------------------

rtl8188fu_init_file = output/rtl8188fu.sh

define rtl8188fu_finalize_hook
	$(Q)cp devices/wireless/rtl8188FU_linux_v5.15.3/8188fu.ko output/rtl8188fu.ko
	$(Q)echo 'insmod rtl8188fu.ko \' > $(rtl8188fu_init_file)
	$(Q)echo 'power_on=$(MD_RTL8188FU_WLAN_POWER_ON) \' >> $(rtl8188fu_init_file)

	$(Q)echo '' >> $(rtl8188fu_init_file)
endef
