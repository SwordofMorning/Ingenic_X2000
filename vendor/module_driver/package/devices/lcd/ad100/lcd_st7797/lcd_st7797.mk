#-------------------------------------------------------
package_name = lcd_st7797
package_depends = utils soc_fb
package_module_src = devices/lcd/ad100/lcd_st7797
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7797_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7797_init_file = output/lcd_st7797.sh

define lcd_st7797_finalize_hook
	$(Q)cp devices/lcd/ad100/lcd_st7797/lcd_st7797.ko output/
	$(Q)echo -n 'insmod lcd_st7797.ko' > $(lcd_st7797_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_AD100_ST7797_POWER_EN)' >> $(lcd_st7797_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_AD100_ST7797_RST)' >> $(lcd_st7797_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_AD100_ST7797_BACKLIGHT_EN)' >> $(lcd_st7797_init_file)
	$(Q)echo -n ' lcd_regulator_name=$(MD_AD100_ST7797_REGULATOR_NAME)' >> $(lcd_st7797_init_file)
	$(Q)echo  >> $(lcd_st7797_init_file)
endef