#-------------------------------------------------------
package_name = wq9001
package_depends = utils
package_module_src = devices/wireless/wq9001/
package_make_hook =
package_init_hook =
package_finalize_hook = wq9001_finalize_hook
package_clean_hook =
#-------------------------------------------------------

wq9001_init_file = output/wq9001.sh

define wq9001_finalize_hook
	$(Q)cp devices/wireless/wq9001/wq9001.ko output/
	$(Q)echo 'insmod wq9001.ko' > $(wq9001_init_file)
endef