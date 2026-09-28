#-------------------------------------------------------
package_name = lcd_ld047rf1l01
package_depends = utils soc_fb
package_module_src = devices/lcd/ad100/lcd_ld047rf1l01
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_ld047rf1l01_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_ld047rf1l01_init_file = output/lcd_ld047rf1l01.sh

define lcd_ld047rf1l01_finalize_hook
	$(Q)cp devices/lcd/ad100/lcd_ld047rf1l01/lcd_ld047rf1l01.ko output/
	$(Q)echo -n 'insmod lcd_ld047rf1l01.ko' > $(lcd_ld047rf1l01_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_AD100_LD047RF1L01_POWER_EN)' >> $(lcd_ld047rf1l01_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_AD100_LD047RF1L01_RST)' >> $(lcd_ld047rf1l01_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_AD100_LD047RF1L01_BACKLIGHT_EN)' >> $(lcd_ld047rf1l01_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_AD100_LD047RF1L01_REGULATOR_NAME)' >> $(lcd_ld047rf1l01_init_file)
	$(Q)echo  >> $(lcd_ld047rf1l01_init_file)
endef