#-------------------------------------------------------
package_name = sensor_bf20a6_dvp
package_depends = utils soc_camera
package_module_src = devices/camera/x2600/bf20a6_dvp
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_bf20a6_dvp_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_bf20a6_dvp.sh


define sensor_bf20a6_dvp_finalize_hook
	$(if $(MD_X2600_SENSOR_BF20A6_DVP), $(Q)cp devices/camera/x2600/bf20a6_dvp/sensor_bf20a6_dvp.ko output/)
	$(if $(MD_X2600_SENSOR_BF20A6_DVP), $(Q)echo -n 'insmod sensor_bf20a6_dvp.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2600_SENSOR_BF20A6_DVP), $(Q)echo -n ' power_gpio=$(MD_X2600_BF20A6_DVP_GPIO_POWER)' >> $(shell_sensor_init_file))
	$(if $(MD_X2600_SENSOR_BF20A6_DVP), $(Q)echo -n ' reset_gpio=$(MD_X2600_BF20A6_DVP_GPIO_RESET)' >> $(shell_sensor_init_file))
	$(if $(MD_X2600_SENSOR_BF20A6_DVP), $(Q)echo -n ' pwdn_gpio=$(MD_X2600_BF20A6_DVP_GPIO_PWDN)' >> $(shell_sensor_init_file))
	$(if $(MD_X2600_SENSOR_BF20A6_DVP), $(Q)echo -n ' i2c_bus_num=$(MD_X2600_BF20A6_DVP_I2C_BUSNUM)' >> $(shell_sensor_init_file))

	$(Q)echo  >> $(shell_sensor_init_file)

endef

