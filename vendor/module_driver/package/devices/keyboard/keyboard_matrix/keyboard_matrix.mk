#-------------------------------------------------------
package_name = keyboard_matrix_dev
package_depends = utils
package_module_src = devices/keyboard_matrix
package_make_hook =
package_init_hook =
package_finalize_hook = keyboard_matrix_finalize_hook
package_clean_hook =
#-------------------------------------------------------

keyboard_matrix_init_file = output/keyboard_matrix_dev.sh

define keyboard_matrix_finalize_hook
	$(Q)cp devices/keyboard_matrix/keyboard_matrix_dev.ko output/
	$(Q)echo 'insmod keyboard_matrix_dev.ko ' > $(keyboard_matrix_init_file)
endef
