#-------------------------------------------------------
package_name = eta6919
package_depends = utils
package_module_src = devices/pmu/eta6919/
package_make_hook =
package_init_hook =
package_finalize_hook = eta6919_finalize_hook
package_clean_hook =
#-------------------------------------------------------

eta6919_init_file = output/eta6919.sh

define eta6919_finalize_hook
	$(Q)cp devices/pmu/eta6919/eta6919.ko output/
	$(Q)echo -n 'insmod eta6919.ko' > $(eta6919_init_file)
	$(Q)echo -n ' pmu_int_gpio=$(MD_PMU_ETA6919_INT_GPIO)' >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_i2c_bus_num=$(MD_PMU_ETA6919_I2C_BUSNUM)' >> $(eta6919_init_file)
	$(Q)echo "\\" >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_mode=$(MD_PMU_ETA6919_MODE)' >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_input_current=$(MD_PMU_ETA6919_INPUT_CURRENT)' >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_input_voltage=$(MD_PMU_ETA6919_INPUT_VOLTAGE)' >> $(eta6919_init_file)
	$(Q)echo "\\" >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_charge_current=$(MD_PMU_ETA6919_CHARGE_CURRENT)' >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_charge_voltage=$(MD_PMU_ETA6919_CHARGE_VOLTAGE)' >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_charge_pre_current=$(MD_PMU_ETA6919_CHARGE_PRE_CURRENT)' >> $(eta6919_init_file)
	$(Q)echo -n ' pmu_charge_term_current=$(MD_PMU_ETA6919_CHARGE_TERM_CURRENT)' >> $(eta6919_init_file)
	$(Q)echo  >> $(eta6919_init_file)
endef
