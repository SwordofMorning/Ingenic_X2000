#-------------------------------------------------------
package_name = fcs950uab_xbt
package_depends = utils soc_msc fcs950uab_uwe5622_sdio
package_module_src = devices/wireless/fcs950uab/unisocbt/
package_make_hook =
package_init_hook =
package_finalize_hook = fcs950uab_xbt_finalize_hook
package_clean_hook =
#-------------------------------------------------------

fcs950uab_xbt_init_file = output/fcs950uab_xbt.sh

define fcs950uab_xbt_finalize_hook
	$(Q)cp devices/wireless/fcs950uab/unisocbt/fcs950uab_xbt.ko output/
	$(Q)echo 'insmod fcs950uab_xbt.ko \' > $(fcs950uab_xbt_init_file)
endef