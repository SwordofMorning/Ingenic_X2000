#-------------------------------------------------------
package_name = lcd_jlt7003a
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_jlt7003a
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_jlt7003a_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_jlt7003a_init_file = output/lcd_jlt7003a.sh

define lcd_jlt7003a_finalize_hook
    $(Q)cp devices/lcd/x2600/lcd_jlt7003a/lcd_jlt7003a.ko output/
    $(Q)echo -n 'insmod lcd_jlt7003a.ko' > $(lcd_jlt7003a_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_JLT7003A_POWER_EN)' >> $(lcd_jlt7003a_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_JLT7003A_RST)' >> $(lcd_jlt7003a_init_file)
    $(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2600_JLT7003A_LCD_BACKLIGHT_ENABLE)' >> $(lcd_jlt7003a_init_file)
    $(Q)echo -n ' lcd_regulator_name=$(MD_X2600_JLT7003A_REGULATOR_NAME)' >> $(lcd_jlt7003a_init_file)
    $(Q)echo  >> $(lcd_jlt7003a_init_file)
endef
