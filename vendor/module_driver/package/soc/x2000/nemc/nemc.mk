
#-------------------------------------------------------
package_name = soc_nemc
package_depends = utils
package_module_src = soc/x2000/nemc
package_make_hook =
package_init_hook =
package_finalize_hook = soc_nemc_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_nemc_init_file = output/soc_nemc.sh

define soc_nemc_finalize_hook
	$(Q)cp soc/x2000/nemc/soc_nemc.ko output/
	$(Q)echo -n 'insmod soc_nemc.ko ' > $(soc_nemc_init_file)
	$(Q)echo -n 'nemc0_enable=$(if $(MD_X2000_NEMC0),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc0_buswidth=$(if $(MD_X2000_NEMC0_BUSWIDTH),$(MD_X2000_NEMC0_BUSWIDTH),0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc1_enable=$(if $(MD_X2000_NEMC1),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc1_buswidth=$(if $(MD_X2000_NEMC1_BUSWIDTH),$(MD_X2000_NEMC1_BUSWIDTH),0) ' >> $(soc_nemc_init_file)
	$(Q)echo \\ >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr0=$(if $(MD_X2000_NEMC_ADDR0_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr1=$(if $(MD_X2000_NEMC_ADDR1_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr2=$(if $(MD_X2000_NEMC_ADDR2_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr3=$(if $(MD_X2000_NEMC_ADDR3_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo \\ >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr4=$(if $(MD_X2000_NEMC_ADDR4_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr5=$(if $(MD_X2000_NEMC_ADDR5_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr6=$(if $(MD_X2000_NEMC_ADDR6_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr7=$(if $(MD_X2000_NEMC_ADDR7_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo \\ >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr8=$(if $(MD_X2000_NEMC_ADDR8_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr9=$(if $(MD_X2000_NEMC_ADDR9_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr10=$(if $(MD_X2000_NEMC_ADDR10_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo -n 'nemc_addr11=$(if $(MD_X2000_NEMC_ADDR11_GPIO),1,0) ' >> $(soc_nemc_init_file)
	$(Q)echo 'nemc_addr12=$(if $(MD_X2000_NEMC_ADDR12_GPIO),1,0)' >> $(soc_nemc_init_file)
endef
