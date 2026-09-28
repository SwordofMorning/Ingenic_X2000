#-------------------------------------------------------
package_name = lcd_gm8285c
package_depends = utils soc_fb
package_module_src = devices/lcd/x2000/lcd_gm8285c
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_gm8285c_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_gm8285c_init_file = output/lcd_gm8285c.sh

define lcd_gm8285c_finalize_hook
	$(Q)cp devices/lcd/x2000/lcd_gm8285c/lcd_gm8285c.ko output/
	$(Q)echo -n 'insmod lcd_gm8285c.ko' > $(lcd_gm8285c_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2000_GM8285C_POWER_EN)' >> $(lcd_gm8285c_init_file)
	$(Q)echo -n ' power_valid_level=$(MD_X2000_GM8285C_POWER_VALID_LEVEL) \' >> $(lcd_gm8285c_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2000_GM8285C_RST)' >> $(lcd_gm8285c_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_X2000_GM8285C_REGULATOR_NAME)' >> $(lcd_gm8285c_init_file)
	$(Q)echo  >> $(lcd_gm8285c_init_file)
endef