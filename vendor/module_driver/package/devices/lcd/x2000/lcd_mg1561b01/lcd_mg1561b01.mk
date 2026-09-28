
#-------------------------------------------------------
package_name = lcd_mg1561b01
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_mg1561b01
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_mg1561b01_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_mg1561b01_init_file = output/lcd_mg1561b01.sh

define lcd_mg1561b01_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_mg1561b01/lcd_mg1561b01.ko output/
	$(Q)echo -n 'insmod lcd_mg1561b01.ko' > $(lcd_mg1561b01_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_MG1561B01_POWER_EN)' >> $(lcd_mg1561b01_init_file)
	$(Q)echo -n ' power_valid_level=$(MD_X2000_MG1561B01_POWER_VALID_LEVEL) \' >> $(lcd_mg1561b01_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_MG1561B01_RST)' >> $(lcd_mg1561b01_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_X2000_MG1561B01_REGULATOR_NAME)' >> $(lcd_mg1561b01_init_file)
	$(Q)echo -n ' mg1561b01_i2c_bus_num=$(MD_X2000_MG1561B01_I2C_BUS_NUM)' >> $(lcd_mg1561b01_init_file)
	$(Q)echo  >> $(lcd_mg1561b01_init_file)
endef
