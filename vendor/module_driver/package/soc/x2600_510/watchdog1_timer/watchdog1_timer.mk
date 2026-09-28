#-------------------------------------------------------
package_name = soc_watchdog1_timer
package_depends = utils
package_module_src = soc/x2600_510/watchdog1_timer
package_make_hook =
package_init_hook =
package_finalize_hook = watchdog1_timer_finalize_hook
package_clean_hook =
#-------------------------------------------------------

watchdog1_timer_init_file = output/soc_watchdog1_timer.sh

define watchdog1_timer_finalize_hook
	$(Q)cp soc/x2600_510/watchdog1_timer/soc_watchdog1_timer.ko output/
	$(Q)echo -n 'insmod soc_watchdog1_timer.ko ' > $(watchdog1_timer_init_file)

	$(Q)echo >> $(watchdog1_timer_init_file)
endef