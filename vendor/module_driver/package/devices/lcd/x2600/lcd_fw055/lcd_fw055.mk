#-------------------------------------------------------
package_name = lcd_fw055
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_fw055
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_fw055_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_fw055_init_file = output/lcd_fw055.sh

define lcd_fw055_finalize_hook
	$(Q)cp devices/lcd/x2600/lcd_fw055/lcd_fw055.ko output/
	$(Q)echo -n 'insmod lcd_fw055.ko' > $(lcd_fw055_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_FW055_POWER_EN)' >> $(lcd_fw055_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_FW055_RST)' >> $(lcd_fw055_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2600_FW055_BACKLIGHT_EN)' >> $(lcd_fw055_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_X2600_FW055_REGULATOR_NAME)' >> $(lcd_fw055_init_file)
	$(Q)echo  >> $(lcd_fw055_init_file)
endef