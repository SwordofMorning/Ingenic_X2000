
#-------------------------------------------------------
package_name = lcd_gc9503v
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_gc9503v
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_gc9503v_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_gc9503v_init_file = output/lcd_gc9503v.sh

define lcd_gc9503v_finalize_hook
    $(Q)cp devices/lcd/x2600/lcd_gc9503v/lcd_gc9503v.ko output/
    $(Q)echo -n 'insmod lcd_gc9503v.ko' > $(lcd_gc9503v_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_GC9503V_POWER_EN)' >> $(lcd_gc9503v_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_GC9503V_RST)' >> $(lcd_gc9503v_init_file)
    $(Q)echo -n ' gpio_lcd_scl=$(MD_X2600_GC9503V_SPI_SCL)' >> $(lcd_gc9503v_init_file)
    $(Q)echo -n ' gpio_lcd_sda=$(MD_X2600_GC9503V_SPI_SDA)' >> $(lcd_gc9503v_init_file)
    $(Q)echo -n ' gpio_lcd_cs=$(MD_X2600_GC9503V_SPI_CS)' >> $(lcd_gc9503v_init_file)
    $(Q)echo  >> $(lcd_gc9503v_init_file)
endef
