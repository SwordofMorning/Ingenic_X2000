#-------------------------------------------------------
package_name = soc_mcu
package_depends = utils
package_module_src = soc/x2600_510/mcu_riscv
package_make_hook =
package_init_hook =
package_finalize_hook = mcu_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_mcu_init_file = output/soc_mcu.sh

define mcu_finalize_hook
	$(Q)cp soc/x2600_510/mcu_riscv/soc_mcu.ko output/
	$(Q)echo -n 'insmod soc_mcu.ko ' > $(soc_mcu_init_file)
	$(Q)echo -n 'enable_jtag_debug=$(if $(MD_X2600_510_MCU_ENABLE_JTAG),1,0)' >> $(soc_mcu_init_file)
	$(Q)echo >> $(soc_mcu_init_file)
endef