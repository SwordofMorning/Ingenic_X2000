#-------------------------------------------------------
package_name = sensor_og02b10_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2580/og02b10_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_og02b10_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

shell_sensor_init_file = output/sensor_og02b10_mipi.sh

define sensor_og02b10_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/camera/x2580/og02b10_mipi/og02b10-x2580.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_og02b10_mipi_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/og02b10-x2580.bin
endef

TARGET_INSTALL_HOOKS += sensor_og02b10_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_og02b10_mipi_install_clean_hook

define sensor_og02b10_mipi_finalize_hook
	$(Q)cp devices/camera/x2580/og02b10_mipi/sensor_og02b10_mipi.ko output/
	$(Q)echo -n 'insmod sensor_og02b10_mipi.ko' > $(shell_sensor_init_file)
	$(Q)echo -n ' sensor_name=$(MD_X2580_OG02B10_SENSOR_NAME)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2580_OG02B10_GPIO_POWER)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2580_OG02B10_GPIO_PWDN)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2580_OG02B10_I2C_BUSNUM)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' i2c_addr=$(MD_X2580_OG02B10_I2C_ADDR)' >> $(shell_sensor_init_file)
	$(Q)echo -n ' cam_bus_num=$(MD_X2580_OG02B10_CAM_BUSNUM)' >> $(shell_sensor_init_file)

	$(Q)echo  >> $(shell_sensor_init_file)
endef
