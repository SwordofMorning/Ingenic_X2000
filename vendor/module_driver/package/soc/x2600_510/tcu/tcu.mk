#-------------------------------------------------------
package_name = soc_tcu
package_depends = utils
package_module_src = soc/x2600_510/tcu
package_make_hook =
package_init_hook =
package_finalize_hook = tcu_finalize_hook
package_clean_hook =
#-------------------------------------------------------

tcu_init_file = output/soc_tcu.sh

define tcu_finalize_hook
    $(Q)cp soc/x2600_510/tcu/soc_tcu.ko output/
    $(Q)echo "insmod soc_tcu.ko \\" > $(tcu_init_file)
    $(Q)echo -n "    tcu0_is_enable=$(if $(MD_X2600_510_TCU0),1,0) " >> $(tcu_init_file)
    $(Q)echo -n "    tcu1_is_enable=$(if $(MD_X2600_510_TCU1),1,0) " >> $(tcu_init_file)
    $(Q)echo >> $(tcu_init_file)
endef