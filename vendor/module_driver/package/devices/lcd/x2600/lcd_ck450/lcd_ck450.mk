#-------------------------------------------------------
package_name = lcd_ck450
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_ck450
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_ck450_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_ck450_init_file = output/lcd_ck450.sh

define lcd_ck450_finalize_hook
    $(Q)cp devices/lcd/x2600/lcd_ck450/lcd_ck450.ko output/
    $(Q)echo -n 'insmod lcd_ck450.ko' > $(lcd_ck450_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_CK450_POWER_EN)' >> $(lcd_ck450_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_CK450_RST)' >> $(lcd_ck450_init_file)
    $(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2600_CK450_LCD_BACKLIGHT_ENABLE)' >> $(lcd_ck450_init_file)
    $(Q)echo -n ' lcd_regulator_name=$(MD_X2600_CK450_REGULATOR_NAME)' >> $(lcd_ck450_init_file)
    $(Q)echo  >> $(lcd_ck450_init_file)
endef
