#-------------------------------------------------------
package_name = lcd_st7701s_frd450
package_depends = utils soc_fb
package_module_src = devices/lcd/ad100/lcd_st7701s_frd450
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7701s_frd450_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7701s_frd450_init_file = output/lcd_st7701s_frd450.sh

define lcd_st7701s_frd450_finalize_hook
	$(Q)cp devices/lcd/ad100/lcd_st7701s_frd450/lcd_st7701s_frd450.ko output/
	$(Q)echo -n 'insmod lcd_st7701s_frd450.ko' > $(lcd_st7701s_frd450_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_AD100_ST7701S_FRD450_POWER_EN)' >> $(lcd_st7701s_frd450_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_AD100_ST7701S_FRD450_RST)' >> $(lcd_st7701s_frd450_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_AD100_ST7701S_FRD450_BACKLIGHT_EN)' >> $(lcd_st7701s_frd450_init_file)
	$(Q)echo -n ' lcd_regulator_name=$(MD_AD100_ST7701S_FRD450_REGULATOR_NAME)' >> $(lcd_st7701s_frd450_init_file)
	$(Q)echo  >> $(lcd_st7701s_frd450_init_file)
endef