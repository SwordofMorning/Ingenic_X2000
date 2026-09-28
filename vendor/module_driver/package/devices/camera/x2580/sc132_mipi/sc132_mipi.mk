#-------------------------------------------------------
package_name = sensor_sc132_mipi
package_depends = utils soc_camera
package_module_src = devices/camera/x2580/sc132_mipi
package_make_hook =
package_init_hook =
package_finalize_hook = sensor_sc132_mipi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sensor_sc132_mipi_init_file = output/sensor_sc132_mipi.sh


define sensor_sc132_mipi_install_hook
	$(Q)mkdir -p $(FS_TARGET_DIR)/etc/sensor
	$(Q)cp -rf package/devices/camera/x2580/sc132_mipi/sc132-x2580.bin $(FS_TARGET_DIR)/etc/sensor/
endef

define sensor_sc132_mipi_install_clean_hook
	$(Q)rm -rf $(FS_TARGET_DIR)/etc/sensor/sc132-x2580.bin
endef

TARGET_INSTALL_HOOKS += sensor_sc132_mipi_install_hook
TARGET_INSTALL_CLEAN_HOOKS += sensor_sc132_mipi_install_clean_hook

define sensor_sc132_mipi_finalize_hook
	$(Q)cp devices/camera/x2580/sc132_mipi/sensor_sc132_mipi.ko output/
	$(Q)echo -n 'insmod sensor_sc132_mipi.ko' > $(sensor_sc132_mipi_init_file)
	$(Q)echo -n ' power_gpio=$(MD_X2580_SC132_GPIO_POWER)' >> $(sensor_sc132_mipi_init_file)
	$(Q)echo -n ' reset_gpio=$(MD_X2580_SC132_GPIO_RESET)' >> $(sensor_sc132_mipi_init_file)
	$(Q)echo -n ' pwdn_gpio=$(MD_X2580_SC132_GPIO_PWDN)' >> $(sensor_sc132_mipi_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2580_SC132_I2C_BUSNUM)' >> $(sensor_sc132_mipi_init_file)
	$(Q)echo  >> $(sensor_sc132_mipi_init_file)
endef
