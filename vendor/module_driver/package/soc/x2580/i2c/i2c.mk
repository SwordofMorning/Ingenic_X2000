
#-------------------------------------------------------
package_name = soc_i2c
package_depends = utils
package_module_src = soc/x2580/i2c/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_i2c_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_i2c_init_file = output/soc_i2c.sh

define soc_i2c_finalize_hook
	$(Q)cp soc/x2580/i2c/soc_i2c.ko output/
	$(Q)echo "insmod soc_i2c.ko \\" > $(soc_i2c_init_file)

	$(Q)echo -n "	i2c0_is_enable=$(if $(MD_X2580_I2C0_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_rate=$(if $(MD_X2580_I2C0_RATE),$(MD_X2580_I2C0_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_scl=PA13 " >> $(soc_i2c_init_file)
	$(Q)echo "i2c0_sda=PA12 \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c1_is_enable=$(if $(MD_X2580_I2C1_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_rate=$(if $(MD_X2580_I2C1_RATE),$(MD_X2580_I2C1_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_scl=$(if $(MD_X2580_I2C1_PB),PB26,$(if $(MD_X2580_I2C1_PC),PC20,PD07)) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c1_sda=$(if $(MD_X2580_I2C1_PB),PB25,$(if $(MD_X2580_I2C1_PC),PC19,PD06)) \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c2_is_enable=$(if $(MD_X2580_I2C2_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c2_rate=$(if $(MD_X2580_I2C2_RATE),$(MD_X2580_I2C2_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c2_scl=$(if $(MD_X2580_I2C2_PB_10_11),PB11,$(if $(MD_X2580_I2C2_PB_27_28),PB28,PC09)) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c2_sda=$(if $(MD_X2580_I2C2_PB_10_11),PB10,$(if $(MD_X2580_I2C2_PB_27_28),PB27,PC08)) \\" >> $(soc_i2c_init_file)
endef
