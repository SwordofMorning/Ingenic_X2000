#-------------------------------------------------------
package_name = lcd_ms7024
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_ms7024
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_ms7024_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_ms7024_init_file = output/lcd_ms7024.sh

define lcd_ms7024_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_ms7024/lcd_ms7024.ko output/
	$(Q)echo -n 'insmod lcd_ms7024.ko' > $(lcd_ms7024_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_MS7024_POWER_EN)' >> $(lcd_ms7024_init_file)
	$(Q)echo -n ' power_valid_level=$(MD_X2000_MS7024_POWER_VALID_LEVEL) \' >> $(lcd_ms7024_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_MS7024_RST)' >> $(lcd_ms7024_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_X2000_MS7024_REGULATOR_NAME)' >> $(lcd_ms7024_init_file)
	$(Q)echo -n ' ms7024_i2c_bus_num=$(MD_X2000_MS7024_I2C_BUS_NUM)' >> $(lcd_ms7024_init_file)
	$(Q)echo  >> $(lcd_ms7024_init_file)
endef