#-------------------------------------------------------
package_name = lcd_tf450
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_tf450
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_tf450_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_tf450_init_file = output/lcd_tf450.sh

define lcd_tf450_finalize_hook
    $(Q)cp devices/lcd/x2600/lcd_tf450/lcd_tf450.ko output/
    $(Q)echo -n 'insmod lcd_tf450.ko' > $(lcd_tf450_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_TF450_POWER_EN)' >> $(lcd_tf450_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_TF450_RST)' >> $(lcd_tf450_init_file)
    $(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2600_TF450_LCD_BACKLIGHT_ENABLE)' >> $(lcd_tf450_init_file)
    $(Q)echo -n ' lcd_regulator_name=$(MD_X2600_TF450_REGULATOR_NAME)' >> $(lcd_tf450_init_file)
    $(Q)echo  >> $(lcd_tf450_init_file)
endef
