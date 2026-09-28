#-------------------------------------------------------
package_name = lcd_yc101
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_yc101
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_yc101_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_yc101_init_file = output/lcd_yc101.sh

define lcd_yc101_finalize_hook
	$(Q)cp devices/lcd/x2600/lcd_yc101/lcd_yc101.ko output/
	$(Q)echo -n 'insmod lcd_yc101.ko' > $(lcd_yc101_init_file)
	$(Q)echo -n ' lcd_4lane_en=$(if $(MD_X2600_YC101_4LANE_ON),1,0)' >> $(lcd_yc101_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_YC101_POWER_EN)' >> $(lcd_yc101_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_YC101_RST)' >> $(lcd_yc101_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_X2600_YC101_REGULATOR_NAME)' >> $(lcd_yc101_init_file)
	$(Q)echo  >> $(lcd_yc101_init_file)
endef
