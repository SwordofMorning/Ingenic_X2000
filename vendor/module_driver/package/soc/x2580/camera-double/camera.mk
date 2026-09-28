
#-------------------------------------------------------
package_name = soc_camera_double
package_depends = utils rmem_manager
package_module_src = soc/x2580/camera-double
package_make_hook =
package_init_hook =
package_finalize_hook = soc_camera_double_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_camera_double_init_file = output/soc_camera_double.sh

define soc_camera_double_finalize_hook
	$(Q)cp soc/x2580/camera-double/soc_camera_double.ko output/
	$(Q)echo "insmod soc_camera_double.ko \\" > $(soc_camera_double_init_file)

	$(Q)echo "    reset_user_frame_when_stream_on=0 \\" >> $(soc_camera_double_init_file)

	$(Q)echo -n "    vic_is_enable=$(if $(MD_X2580_CAMERA_VIC_DOUBLE),1,0 ) " >> $(soc_camera_double_init_file)
	$(Q)echo -n "vic_is_isp_enable=$(if $(MD_X2580_CAMERA_VIC_DOUBLE_ISP_ENABLE),1,0 ) " >> $(soc_camera_double_init_file)
	$(Q)echo -n "vic_mclk_io=$(if $(MD_X2580_CAMERA_VIC_DOUBLE),$(MD_X2580_CAMERA_VIC_DOUBLE_MCLK),-1)  " >> $(soc_camera_double_init_file)
	$(Q)echo -n "vic_mipi_switch_gpio=$(if $(MD_X2580_CAMERA_VIC_DOUBLE_GPIO_MIPI_SWITCH),$(MD_X2580_CAMERA_VIC_DOUBLE_GPIO_MIPI_SWITCH),-1) " >> $(soc_camera_double_init_file)
	$(Q)echo "vic_frame_nums=$(if $(MD_X2580_CAMERA_VIC_DOUBLE_FRAME_NUMS),$(MD_X2580_CAMERA_VIC_DOUBLE_FRAME_NUMS),0 ) \\" >> $(soc_camera_double_init_file)

	$(Q)echo  >> $(soc_camera_double_init_file)

endef
