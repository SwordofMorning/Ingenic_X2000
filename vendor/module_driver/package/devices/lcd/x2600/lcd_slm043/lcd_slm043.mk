
#-------------------------------------------------------
package_name = lcd_slm043
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_slm043
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_slm043_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_slm043_init_file = output/lcd_slm043.sh

define lcd_slm043_finalize_hook
	$(Q)cp devices/lcd/x2600/lcd_slm043/lcd_slm043.ko output/
	$(Q)echo -n 'insmod lcd_slm043.ko' > $(lcd_slm043_init_file)
	$(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_SLM043_POWER_EN)' >> $(lcd_slm043_init_file)
	$(Q)echo  >> $(lcd_slm043_init_file)
endef
