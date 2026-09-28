
#-------------------------------------------------------
package_name = soc_rsa
package_depends =
package_module_src = soc/x2580/rsa/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_rsa_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_rsa_init_file = output/soc_rsa.sh

define soc_rsa_finalize_hook
	$(Q)cp soc/x2580/rsa/soc_rsa.ko output/
	$(Q)echo 'insmod soc_rsa.ko ' > $(soc_rsa_init_file)
endef
