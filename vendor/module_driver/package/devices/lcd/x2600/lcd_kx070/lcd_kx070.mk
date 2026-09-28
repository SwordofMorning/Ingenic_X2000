#-------------------------------------------------------
package_name = lcd_kx070
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_kx070
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_kx070_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_kx070_init_file = output/lcd_kx070.sh

define lcd_kx070_finalize_hook
    $(Q)cp devices/lcd/x2600/lcd_kx070/lcd_kx070.ko output/
    $(Q)echo -n 'insmod lcd_kx070.ko' > $(lcd_kx070_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_KX070_POWER_EN)' >> $(lcd_kx070_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_KX070_RST)' >> $(lcd_kx070_init_file)
    $(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2600_KX070_LCD_BACKLIGHT_ENABLE)' >> $(lcd_kx070_init_file)
    $(Q)echo -n ' lcd_regulator_name=$(MD_X2600_KX070_REGULATOR_NAME)' >> $(lcd_kx070_init_file)
    $(Q)echo  >> $(lcd_kx070_init_file)
endef
