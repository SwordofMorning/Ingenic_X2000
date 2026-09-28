#-------------------------------------------------------
package_name = cst8xx_touch
package_depends = utils
package_module_src = devices/touchscreen/cst8xx_touch/
package_make_hook =
package_init_hook =
package_finalize_hook = cst8xx_touch_finalize_hook
package_clean_hook =
#-------------------------------------------------------

cst8xx_touch_init_file = output/cst8xx_touch.sh

define cst8xx_touch_finalize_hook
	$(Q)cp devices/touchscreen/cst8xx_touch/cst8xx_touch.ko output/
	$(Q)echo "insmod cst8xx_touch.ko \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_i2c_bus_num=$(MD_CST8_I2C_BUSNUM) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_x_coords_min=$(MD_CST8_X_COORDS_MIN) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_y_coords_min=$(MD_CST8_Y_COORDS_MIN) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_x_coords_max=$(MD_CST8_X_COORDS_MAX) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_y_coords_max=$(MD_CST8_Y_COORDS_MAX) \\" >> $(cst8xx_touch_init_file)

	$(Q)echo " cst_x_coords_flip=$(if $(MD_CST8_X_COORDS_FLIP),1,0) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_y_coords_flip=$(if $(MD_CST8_Y_COORDS_FLIP),1,0) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_x_y_coords_exchange=$(if $(MD_CST8_X_Y_COORDS_EXCHANGE),1,0) \\" >> $(cst8xx_touch_init_file)

	$(Q)echo " cst_reset_gpio=$(MD_CST8_RESET_GPIO) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_irq_gpio=$(MD_CST8_IRQ_GPIO) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_power_en_gpio=$(MD_CST8_I2C_POWER_ENABLE_GPIO) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_power_en_level=$(MD_CST8_I2C_POWER_ENABLE_LEVEL) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_max_touch_number=$(MD_CST8_MAX_TOUCH_NUMBER) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo " cst_regulator_name=$(MD_CST8_REGULATOR_NAME) \\" >> $(cst8xx_touch_init_file)
	$(Q)echo >> $(cst8xx_touch_init_file)
endef