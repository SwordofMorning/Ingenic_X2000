
#-------------------------------------------------------
package_name = soc_camera
package_depends = utils rmem_manager
package_module_src = soc/x2580/camera
package_make_hook =
package_init_hook =
package_finalize_hook = soc_camera_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_camera_init_file = output/soc_camera.sh

define soc_camera_finalize_hook
	$(Q)cp soc/x2580/camera/soc_camera.ko output/
	$(Q)echo "insmod soc_camera.ko \\" > $(soc_camera_init_file)

	$(Q)echo "    reset_user_frame_when_stream_on=0 \\" >> $(soc_camera_init_file)

	$(Q)echo -n "    vic_is_enable=$(if $(MD_X2580_CAMERA_VIC),1,0 ) " >> $(soc_camera_init_file)
	$(Q)echo -n "vic_is_isp_enable=$(if $(MD_X2580_CAMERA_VIC_ISP_ENABLE),1,0 ) " >> $(soc_camera_init_file)
	$(Q)echo -n "vic_mclk_io=$(if $(MD_X2580_CAMERA_VIC),$(MD_X2580_CAMERA_VIC_MCLK),-1)  " >> $(soc_camera_init_file)
	$(Q)echo "vic_frame_nums=$(if $(MD_X2580_CAMERA_VIC_FRAME_NUMS),$(MD_X2580_CAMERA_VIC_FRAME_NUMS),0 ) \\" >> $(soc_camera_init_file)

	$(Q)echo  >> $(soc_camera_init_file)

endef
