
#-------------------------------------------------------
package_name = lcd_st7701s_tft
package_depends = utils soc_fb
package_module_src = devices/lcd/x2600/lcd_st7701s_tft
package_make_hook =
package_init_hook =
package_finalize_hook = lcd_st7701s_tft_finalize_hook
package_clean_hook =
#-------------------------------------------------------

lcd_st7701s_tft_init_file = output/lcd_st7701s_tft.sh

define lcd_st7701s_tft_finalize_hook
    $(Q)cp devices/lcd/x2600/lcd_st7701s_tft/lcd_st7701s_tft.ko output/
    $(Q)echo -n 'insmod lcd_st7701s_tft.ko' > $(lcd_st7701s_tft_init_file)
    $(Q)echo -n ' gpio_lcd_power_en=$(MD_X2600_ST7701S_TFT_POWER_EN)' >> $(lcd_st7701s_tft_init_file)
    $(Q)echo -n ' gpio_lcd_rst=$(MD_X2600_ST7701S_TFT_RST)' >> $(lcd_st7701s_tft_init_file)
    $(Q)echo -n ' gpio_lcd_scl=$(MD_X2600_ST7701S_TFT_SPI_SCL)' >> $(lcd_st7701s_tft_init_file)
    $(Q)echo -n ' gpio_lcd_sda=$(MD_X2600_ST7701S_TFT_SPI_SDA)' >> $(lcd_st7701s_tft_init_file)
    $(Q)echo -n ' gpio_lcd_cs=$(MD_X2600_ST7701S_TFT_SPI_CS)' >> $(lcd_st7701s_tft_init_file)
    $(Q)echo  >> $(lcd_st7701s_tft_init_file)
endef