
#-------------------------------------------------------
package_name = soc_i2c
package_depends = utils
package_module_src = soc/ad100_510/i2c/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_i2c_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_i2c_init_file = output/soc_i2c.sh

define soc_i2c_finalize_hook
	$(Q)cp soc/ad100_510/i2c/soc_i2c.ko output/
	$(Q)echo "insmod soc_i2c.ko \\" > $(soc_i2c_init_file)

	$(Q)echo -n "	i2c0_is_enable=$(if $(MD_AD100_510_I2C0_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_rate=$(if $(MD_AD100_510_I2C0_RATE),$(MD_AD100_510_I2C0_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_scl=$(if $(MD_AD100_510_I2C0_PD),PD04,PC00) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c0_sda=$(if $(MD_AD100_510_I2C0_PD),PD03,PC01) \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c1_is_enable=$(if $(MD_AD100_510_I2C1_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_rate=$(if $(MD_AD100_510_I2C1_RATE),$(MD_AD100_510_I2C1_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_scl=PC02 " >> $(soc_i2c_init_file)
	$(Q)echo "i2c1_sda=PC03 \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c2_is_enable=$(if $(MD_AD100_510_I2C2_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c2_rate=$(if $(MD_AD100_510_I2C2_RATE),$(MD_AD100_510_I2C2_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c2_scl=$(if $(MD_AD100_510_I2C2_PB),PB03,PC19) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c2_sda=$(if $(MD_AD100_510_I2C2_PB),PB02,PC20) \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c3_is_enable=$(if $(MD_AD100_510_I2C3_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c3_rate=$(if $(MD_AD100_510_I2C3_RATE),$(MD_AD100_510_I2C3_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c3_scl=PE05 " >> $(soc_i2c_init_file)
	$(Q)echo "i2c3_sda=PE06 " >> $(soc_i2c_init_file)

endef
