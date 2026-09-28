#-------------------------------------------------------
package_name = lcd_qm047ks02
package_depends = utils soc_fb
package_module_src = devices/lcd/ad100/lcd_qm047ks02
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_qm047ks02_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_qm047ks02_init_file = output/lcd_qm047ks02.sh

define lcd_qm047ks02_finalize_hook
	$(Q)cp devices/lcd/ad100/lcd_qm047ks02/lcd_qm047ks02.ko output/
	$(Q)echo -n 'insmod lcd_qm047ks02.ko' > $(lcd_qm047ks02_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_AD100_QM047KS02_POWER_EN)' >> $(lcd_qm047ks02_init_file)
	$(Q)echo -n ' gpio_lcd_rst=$(MD_AD100_QM047KS02_RST)' >> $(lcd_qm047ks02_init_file)
	$(Q)echo -n ' gpio_lcd_backlight_en=$(MD_AD100_QM047KS02_BACKLIGHT_EN)' >> $(lcd_qm047ks02_init_file)
	$(Q)echo -n ' gpio_lcd_regulator_name=$(MD_AD100_QM047KS02_REGULATOR_NAME)' >> $(lcd_qm047ks02_init_file)
	$(Q)echo  >> $(lcd_qm047ks02_init_file)
endef