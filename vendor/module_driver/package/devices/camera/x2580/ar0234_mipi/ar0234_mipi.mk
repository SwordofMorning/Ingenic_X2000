#-------------------------------------------------------
package_name = sensor_ar0234_mipi
package_depends = utils soc_camera_double
package_module_src = devices/camera/x2580/ar0234_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_ar0234_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_ar0234_mipi.sh


define sensor_ar0234_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/camera/x2580/ar0234_mipi/ar0234-x2580.bin $(FS_TARGET_DIR)/etc/sensor/ar0234-0-x2580.bin
	$(Q)cp -rf package/devices/camera/x2580/ar0234_mipi/ar0234-x2580.bin $(FS_TARGET_DIR)/etc/sensor/ar0234-1-x2580.bin
endef

define sensor_ar0234_mipi_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ar0234-0-x2580.bin
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/ar0234-1-x2580.bin
endef

TARGET_INSTALL_HOOKS += sensor_ar0234_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_ar0234_mipi_install_clean_hook


define sensor_ar0234_mipi_finalize_hook
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)cp devices/camera/x2580/ar0234_mipi/sensor0_ar0234_mipi.ko output/)
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)echo -n 'insmod sensor0_ar0234_mipi.ko' > $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)echo -n ' sensor_name=$(MD_X2580_AR0234_SENSOR0_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)echo -n ' power_gpio=$(MD_X2580_AR0234_GPIO_POWER0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)echo -n ' reset_gpio=$(MD_X2580_AR0234_GPIO_RESET0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)echo -n ' regulator_name=$(MD_X2580_AR0234_REGULATOR_NAME0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)echo -n ' i2c_bus_num=$(MD_X2580_AR0234_I2C_BUSNUM0)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR0_AR0234), $(Q)echo -n ' i2c_addr=$(MD_X2580_AR0234_I2C_ADDR0)' >> $(shell_sensor_init_file))

	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo  >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)cp devices/camera/x2580/ar0234_mipi/sensor1_ar0234_mipi.ko output/)
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo -n 'insmod sensor1_ar0234_mipi.ko' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo -n ' sensor_name=$(MD_X2580_AR0234_SENSOR1_NAME)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo -n ' power_gpio=$(MD_X2580_AR0234_GPIO_POWER1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo -n ' reset_gpio=$(MD_X2580_AR0234_GPIO_RESET1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo -n ' regulator_name=$(MD_X2580_AR0234_REGULATOR_NAME1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo -n ' i2c_bus_num=$(MD_X2580_AR0234_I2C_BUSNUM1)' >> $(shell_sensor_init_file))
	$(if $(MD_X2580_SENSOR1_AR0234), $(Q)echo -n ' i2c_addr=$(MD_X2580_AR0234_I2C_ADDR1)' >> $(shell_sensor_init_file))
	$(Q)echo  >> $(shell_sensor_init_file)
endef
