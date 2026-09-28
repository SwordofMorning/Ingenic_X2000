#-------------------------------------------------------
package_name = lcd_qm101
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_qm101
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_qm101_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_qm101_init_file = output/lcd_qm101.sh

define lcd_qm101_finalize_hook
    $(Q)cp devices/lcd/x2600/lcd_qm101/lcd_qm101.ko output/
    $(Q)echo -n 'insmod lcd_qm101.ko' > $(lcd_qm101_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_QM101_POWER_EN)' >> $(lcd_qm101_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_QM101_RST)' >> $(lcd_qm101_init_file)
    $(Q)echo -n ' gpio_lcd_backlight_en=$(MD_X2600_QM101_LCD_BACKLIGHT_ENABLE)' >> $(lcd_qm101_init_file)
    $(Q)echo -n ' lcd_regulator_name=$(MD_X2600_QM101_REGULATOR_NAME)' >> $(lcd_qm101_init_file)
    $(Q)echo  >> $(lcd_qm101_init_file)
endef
