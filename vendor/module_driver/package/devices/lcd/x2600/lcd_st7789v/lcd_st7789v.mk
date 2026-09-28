#-------------------------------------------------------
package_name = lcd_st7789v
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_st7789v
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7789v_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7789v_init_file = output/lcd_st7789v.sh

define lcd_st7789v_finalize_hook
	$(Q)cp devices/lcd/x2600/lcd_st7789v/lcd_st7789v.ko output/
	$(Q)echo -n 'insmod lcd_st7789v.ko' > $(lcd_st7789v_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_ST7789V_POWER_EN)' >> $(lcd_st7789v_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_ST7789V_RST)' >> $(lcd_st7789v_init_file)
	$(Q)echo -n ' gpio_lcd_cs=$(MD_X2600_ST7789V_CS)' >> $(lcd_st7789v_init_file)
	$(Q)echo -n ' gpio_lcd_rd=$(MD_X2600_ST7789V_RD)' >> $(lcd_st7789v_init_file)
	$(Q)echo -n ' lcd_regulator_name=$(MD_X2600_LCD_REGULATOR_NAME)' >> $(lcd_st7789v_init_file)
	$(Q)echo  >> $(lcd_st7789v_init_file)
endef