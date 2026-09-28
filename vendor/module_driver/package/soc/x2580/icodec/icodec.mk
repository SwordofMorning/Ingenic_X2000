
#-------------------------------------------------------
package_name = soc_icodec
package_depends =
package_module_src = soc/x2580/icodec/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_icodec_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_icodec_init_file = output/soc_icodec.sh

define soc_icodec_finalize_hook
	$(Q)cp soc/x2580/icodec/soc_icodec.ko output/
	$(Q)echo "insmod soc_icodec.ko \\" > $(soc_icodec_init_file)
	$(Q)echo -n " bias_enable=$(if $(MD_X2580_ICODEC_BAIS_ON),1,0) " >> $(soc_icodec_init_file)
	$(Q)echo -n " bias_level=$(MD_X2580_ICODEC_BAIS_LEVEL) " >> $(soc_icodec_init_file)
	$(Q)echo "\\" >> $(soc_icodec_init_file)
	$(Q)echo -n " max_hpout_volume=$(MD_X2580_ICODEC_MAX_HPOUT_VAL) " >> $(soc_icodec_init_file)
	$(Q)echo -n " min_hpout_volume=$(MD_X2580_ICODEC_MIN_HPOUT_VAL) " >> $(soc_icodec_init_file)
	$(Q)echo "\\" >> $(soc_icodec_init_file)
	$(Q)echo -n ' speaker_gpio=$(MD_X2580_ICODEC_SPK_GPIO) ' >> $(soc_icodec_init_file)
	$(Q)echo -n ' speaker_gpio_level=$(MD_X2580_ICODEC_SPK_GPIO_LEVEL) ' >> $(soc_icodec_init_file)
	$(Q)echo -n ' speaker_need_delay_ms=$(MD_X2580_ICODEC_SPK_NEED_DELAY_MS) ' >> $(soc_icodec_init_file)
	$(Q)echo "" >> $(soc_icodec_init_file)
endef
