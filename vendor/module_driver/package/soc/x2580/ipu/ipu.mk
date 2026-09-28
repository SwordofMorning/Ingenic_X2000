
#-------------------------------------------------------
package_name = soc_ipu
package_depends =
package_module_src = soc/x2580/ipu/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_ipu_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_ipu_init_file = output/soc_ipu.sh

define soc_ipu_finalize_hook
	$(Q)cp soc/x2580/ipu/soc_ipu.ko output/
	$(Q)echo 'insmod soc_ipu.ko ' > $(soc_ipu_init_file)
endef
