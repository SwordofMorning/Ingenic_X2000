
#-------------------------------------------------------
package_name = soc_sslv
package_depends = utils
package_module_src = soc/ad100_510/sslv
package_make_hook =
package_init_hook =
package_finalize_hook = sslv_finalize_hook
package_clean_hook =
#-------------------------------------------------------

sslv_init_file = output/soc_sslv.sh

define sslv_finalize_hook
	$(Q)cp soc/ad100_510/sslv/soc_sslv.ko output/
	$(Q)echo "insmod soc_sslv.ko \\" > $(sslv_init_file)

	$(Q)echo -n "sslv0_is_enable=$(if $(MD_AD100_510_SSLV),1,0) " >> $(sslv_init_file)
	$(Q)echo -n "sslv0_cs=$(if $(MD_AD100_510_SSLV),$(MD_AD100_510_SSLV0_CS),-1) " >> $(sslv_init_file)
	$(Q)echo -n "sslv0_dt=$(if $(MD_AD100_510_SSLV),$(MD_AD100_510_SSLV0_DT),-1) " >> $(sslv_init_file)
	$(Q)echo -n "sslv0_dr=$(if $(MD_AD100_510_SSLV),$(MD_AD100_510_SSLV0_DR),-1) " >> $(sslv_init_file)
	$(Q)echo "sslv0_clk=$(if $(MD_AD100_510_SSLV),$(MD_AD100_510_SSLV0_CLK),-1) \\" >> $(sslv_init_file)
endef
