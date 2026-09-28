#-------------------------------------------------------
package_name = soc_watchdog
package_depends = utils
package_module_src = soc/x2600_510/watchdog
package_make_hook =
package_init_hook =
package_finalize_hook = watchdog_finalize_hook
package_clean_hook =
#-------------------------------------------------------

watchdog_init_file = output/soc_watchdog.sh

define watchdog_finalize_hook
	$(Q)cp soc/x2600_510/watchdog/soc_watchdog.ko output/
	$(Q)echo -n 'insmod soc_watchdog.ko ' > $(watchdog_init_file)

	$(Q)echo -n "	wdt0_is_enable=$(if $(MD_X2600_510_WDT0_BUS),1,0) " >> $(watchdog_init_file)
	$(Q)echo -n "	wdt1_is_enable=$(if $(MD_X2600_510_WDT1_BUS),1,0) " >> $(watchdog_init_file)
	$(Q)echo >> $(watchdog_init_file)
endef