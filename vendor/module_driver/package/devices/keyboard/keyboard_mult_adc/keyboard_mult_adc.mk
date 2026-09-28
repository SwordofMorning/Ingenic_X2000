
#-------------------------------------------------------
package_name = keyboard_mult_adc
package_depends = utils soc_adc
package_module_src = devices/keyboard_mult_adc/
package_make_hook =
package_init_hook =
package_finalize_hook = keyboard_mult_adc_finalize_hook
package_clean_hook =
#-------------------------------------------------------

keyboard_mult_adc_init_file = output/keyboard_mult_adc.sh

to_adc_keyboard_code = $(if $($(1)_CODE),$(shell tools/key_to_code.sh $($(1)_CODE)),-1)
to_adc_keyboard_value = $(if $($(1)_VALUE),$($(1)_VALUE),-1)

keys=$(shell seq 1 8)
define key_cmd
	$(Q)$(foreach i,$(keys),\
		echo -n '	adc$(1)_key$(i)_code=$(call to_adc_keyboard_code,MD_ADC$(1)_KEY$(i)) ' >> $(keyboard_mult_adc_init_file); \
		echo 'adc$(1)_key$(i)_value=$(call to_adc_keyboard_value,MD_ADC$(1)_KEY$(i)) \' >> $(keyboard_mult_adc_init_file); \
	)
endef

define keyboard_mult_adc_cmd
	$(Q)echo    '   adc$(1)=$(if $(MD_KEYBOARD_ADC$(1)),1,0) \' >> $(keyboard_mult_adc_init_file)
	$(Q)echo    '	adc$(1)_init_value=$(MD_ADC$(1)_INIT_VALUE) \' >> $(keyboard_mult_adc_init_file)
	$(Q)echo    '	adc$(1)_channel=$(MD_ADC$(1)_CHANNEL) \' >> $(keyboard_mult_adc_init_file)
	$(Q)echo    '	adc$(1)_deviation=$(MD_ADC$(1)_DEVIATION) \' >> $(keyboard_mult_adc_init_file)
	$(Q)echo    '	adc$(1)_key_detectime=$(MD_ADC$(1)_KEY_DETECTIME) \' >> $(keyboard_mult_adc_init_file)

	$(call key_cmd,$(1))
endef

define keyboard_mult_adc_finalize_hook
	$(Q)cp devices/keyboard_mult_adc/keyboard_mult_adc.ko output/
	$(Q)echo 'insmod keyboard_mult_adc.ko \' > $(keyboard_mult_adc_init_file)

	$(Q)echo    '	debug=$(if $(MD_MULT_ADC_KEY_DEBUG),1,0) \' >> $(keyboard_mult_adc_init_file)

	$(if $(MD_KEYBOARD_ADC0), $(call keyboard_mult_adc_cmd,0))
	$(if $(MD_KEYBOARD_ADC1), $(call keyboard_mult_adc_cmd,1))
	$(if $(MD_KEYBOARD_ADC2), $(call keyboard_mult_adc_cmd,2))
	$(if $(MD_KEYBOARD_ADC3), $(call keyboard_mult_adc_cmd,3))

endef
