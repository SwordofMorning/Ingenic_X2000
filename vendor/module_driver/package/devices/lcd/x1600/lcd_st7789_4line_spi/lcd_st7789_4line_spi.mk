#-------------------------------------------------------
package_name = lcd_st7789_4line_spi
package_depends = utils soc_fb
package_module_src = devices/lcd/x1600/lcd_st7789_4line_spi
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7789_4line_spi_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7789_4line_spi_init_file = output/lcd_st7789_4line_spi.sh

define lcd_st7789_4line_spi_finalize_hook
    $(Q)cp devices/lcd/x1600/lcd_st7789_4line_spi/lcd_st7789_4line_spi.ko output/
    $(Q)echo -n 'insmod lcd_st7789_4line_spi.ko' > $(lcd_st7789_4line_spi_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X1600_ST7789_4LINE_SPI_POWER_EN)' >> $(lcd_st7789_4line_spi_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X1600_ST7789_4LINE_SPI_RST)' >> $(lcd_st7789_4line_spi_init_file)
    $(Q)echo -n ' gpio_spi_cs=$(MD_X1600_ST7789_4LINE_SPI_CS)' >> $(lcd_st7789_4line_spi_init_file)
    $(Q)echo  >> $(lcd_st7789_4line_spi_init_file)
endef