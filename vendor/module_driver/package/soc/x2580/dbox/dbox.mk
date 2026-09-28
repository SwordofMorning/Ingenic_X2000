
#-------------------------------------------------------
package_name = soc_dbox
package_depends =
package_module_src = soc/x2580/dbox/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_dbox_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_dbox_init_file = output/soc_dbox.sh

define soc_dbox_finalize_hook
	$(Q)cp soc/x2580/dbox/soc_dbox.ko output/
	$(Q)echo 'insmod soc_dbox.ko ' > $(soc_dbox_init_file)
endef
