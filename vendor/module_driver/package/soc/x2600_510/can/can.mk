
#-------------------------------------------------------
package_name = soc_can
package_depends = utils
package_module_src = soc/x2600_510/can/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_can_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_can_init_file = output/soc_can.sh

define soc_can_finalize_hook
	$(Q)cp soc/x2600_510/can/soc_can.ko output/
	$(Q)echo "insmod soc_can.ko \\" > $(soc_can_init_file)

	$(Q)echo -n "    can0_is_enable=$(if $(MD_X2600_510_CAN0),1,0) " >> $(soc_can_init_file)
	$(Q)echo -n "can0_dt=$(if if $(MD_X2600_510_CAN0),$(MD_X2600_510_CAN0_TX),-1) " >> $(soc_can_init_file)
	$(Q)echo "can0_dr=$(if if $(MD_X2600_510_CAN0),$(MD_X2600_510_CAN0_RX),-1) \\" >> $(soc_can_init_file)

	$(Q)echo -n "    can1_is_enable=$(if $(MD_X2600_510_CAN1),1,0) " >> $(soc_can_init_file)
	$(Q)echo -n "can1_dt=$(if if $(MD_X2600_510_CAN1),$(MD_X2600_510_CAN1_TX),-1) " >> $(soc_can_init_file)
	$(Q)echo "can1_dr=$(if if $(MD_X2600_510_CAN1),$(MD_X2600_510_CAN1_RX),-1)" >> $(soc_can_init_file)

endef
