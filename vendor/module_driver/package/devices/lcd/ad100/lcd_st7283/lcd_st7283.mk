
#-------------------------------------------------------
package_name = lcd_st7283
package_depends = utils soc_fb
package_module_src = devices/lcd/ad100/lcd_st7283
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7283_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7283_init_file = output/lcd_st7283.sh

define lcd_st7283_finalize_hook
	$(Q)cp devices/lcd/ad100/lcd_st7283/lcd_st7283.ko output/
	$(Q)echo -n 'insmod lcd_st7283.ko' > $(lcd_st7283_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_AD100_ST7283_POWER_EN)' >> $(lcd_st7283_init_file)
	$(Q)echo -n ' gpio_lcd_display_en=$(MD_AD100_ST7283_DISPLAY_EN)' >> $(lcd_st7283_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_AD100_ST7283_BACKLIGHT_EN)' >> $(lcd_st7283_init_file)
	$(Q)echo  >> $(lcd_st7283_init_file)
endef
