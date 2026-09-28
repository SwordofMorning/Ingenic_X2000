
#-------------------------------------------------------
package_name = soc_fb
package_depends = utils soc_fb_layer_mixer
package_module_src = soc/ad100_510/fb
package_make_hook =
package_init_hook =
package_finalize_hook = soc_fb_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_fb_init_file = output/soc_fb.sh

define soc_fb_finalize_hook
	$(Q)cp soc/ad100_510/fb/soc_fb.ko output/
	$(Q)echo "insmod soc_fb.ko \\" > $(soc_fb_init_file)
	$(Q)echo -n " lcd_is_inited=$(if $(MD_AD100_510_RTOS_BOOT_LOGO_FOR_KERNEL),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " pan_display_sync=$(if $(MD_AD100_510_FB_PAN_DISPLAY_SYNC),1,0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " srdma_enable=$(if $(MD_AD100_510_FB_SRDMA_CONFIG),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " srdma_frames=$(if $(MD_AD100_510_FB_SRDMA_CONFIG),$(MD_AD100_510_FB_SRDMA_FRAMES),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " mixer_enable=$(if $(MD_AD100_510_FB_MIXER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " tft_underrun_count=0" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " use_default_order=$(if $(MD_AD100_510_FB_USE_DEFAULT_ORDER),1,0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " rot_enable=$(if $(MD_AD100_510_FB_CFG_ROTATOR),1,0) " >> $(soc_fb_init_file)
	$(Q)echo -n " rot_angle=$(if $(MD_AD100_510_FB_CFG_ROTATOR),$(MD_AD100_510_FB_ROTATOR_ANGLE),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " rot_frames=$(if $(MD_AD100_510_FB_CFG_ROTATOR),$(MD_AD100_510_FB_ROTATOR_FRAMES),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " waittime_us=$(MD_AD100_510_FB_MIPI_CMD_WAITTIME_US) " >> $(soc_fb_init_file)
	$(Q)echo -n " swap_enable=$(if $(MD_AD100_510_FB_CFG_MIPI_SWAP),1,0) " >> $(soc_fb_init_file)
	$(Q)echo -n " swap_clk=$(if $(MD_AD100_510_FB_MIPI_CLK_PIN),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " swap_lane0=$(if $(MD_AD100_510_FB_MIPI_LANE0_PIN),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " swap_lane1=$(if $(MD_AD100_510_FB_MIPI_LANE1_PIN),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " swap_lane2=$(if $(MD_AD100_510_FB_MIPI_LANE2_PIN),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " swap_lane3=$(if $(MD_AD100_510_FB_MIPI_LANE3_PIN),1,0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer0_enable=$(if $(MD_AD100_510_FB_LAYER0_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer0_frames=$(if $(MD_AD100_510_FB_LAYER0_ENABLE),$(MD_AD100_510_FB_LAYER0_FRAMES),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer0_alpha=$(if $(MD_AD100_510_FB_LAYER0_ENABLE),$(MD_AD100_510_FB_LAYER0_ALPHA),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer1_enable=$(if $(MD_AD100_510_FB_LAYER1_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer1_frames=$(if $(MD_AD100_510_FB_LAYER1_ENABLE),$(MD_AD100_510_FB_LAYER1_FRAMES),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer1_alpha=$(if $(MD_AD100_510_FB_LAYER1_ENABLE),$(MD_AD100_510_FB_LAYER1_ALPHA),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer2_enable=$(if $(MD_AD100_510_FB_LAYER2_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer2_frames=$(if $(MD_AD100_510_FB_LAYER2_ENABLE),$(MD_AD100_510_FB_LAYER2_FRAMES),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer2_alpha=$(if $(MD_AD100_510_FB_LAYER2_ENABLE),$(MD_AD100_510_FB_LAYER2_ALPHA),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " layer3_enable=$(if $(MD_AD100_510_FB_LAYER3_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer3_frames=$(if $(MD_AD100_510_FB_LAYER3_ENABLE),$(MD_AD100_510_FB_LAYER3_FRAMES),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " layer3_alpha=$(if $(MD_AD100_510_FB_LAYER3_ENABLE),$(MD_AD100_510_FB_LAYER3_ALPHA),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb0_enable=$(if $(MD_AD100_510_FB_LAYER0_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_width=$(if $(MD_AD100_510_FB_LAYER0_USER_ENABLE),$(MD_AD100_510_FB_LAYER0_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_height=$(if $(MD_AD100_510_FB_LAYER0_USER_ENABLE),$(MD_AD100_510_FB_LAYER0_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_xpos=$(if $(MD_AD100_510_FB_LAYER0_USER_ENABLE),$(MD_AD100_510_FB_LAYER0_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_ypos=$(if $(MD_AD100_510_FB_LAYER0_USER_ENABLE),$(MD_AD100_510_FB_LAYER0_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_scaling_enable=$(if $(MD_AD100_510_FB_LAYER0_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_scaling_width=$(if $(MD_AD100_510_FB_LAYER0_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER0_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb0_scaling_height=$(if $(MD_AD100_510_FB_LAYER0_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER0_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb1_enable=$(if $(MD_AD100_510_FB_LAYER1_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_width=$(if $(MD_AD100_510_FB_LAYER1_USER_ENABLE),$(MD_AD100_510_FB_LAYER1_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_height=$(if $(MD_AD100_510_FB_LAYER1_USER_ENABLE),$(MD_AD100_510_FB_LAYER1_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_xpos=$(if $(MD_AD100_510_FB_LAYER1_USER_ENABLE),$(MD_AD100_510_FB_LAYER1_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_ypos=$(if $(MD_AD100_510_FB_LAYER1_USER_ENABLE),$(MD_AD100_510_FB_LAYER1_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_scaling_enable=$(if $(MD_AD100_510_FB_LAYER1_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_scaling_width=$(if $(MD_AD100_510_FB_LAYER1_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER1_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb1_scaling_height=$(if $(MD_AD100_510_FB_LAYER1_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER1_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb2_enable=$(if $(MD_AD100_510_FB_LAYER2_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_width=$(if $(MD_AD100_510_FB_LAYER2_USER_ENABLE),$(MD_AD100_510_FB_LAYER2_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_height=$(if $(MD_AD100_510_FB_LAYER2_USER_ENABLE),$(MD_AD100_510_FB_LAYER2_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_xpos=$(if $(MD_AD100_510_FB_LAYER2_USER_ENABLE),$(MD_AD100_510_FB_LAYER2_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_ypos=$(if $(MD_AD100_510_FB_LAYER2_USER_ENABLE),$(MD_AD100_510_FB_LAYER2_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_scaling_enable=$(if $(MD_AD100_510_FB_LAYER2_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_scaling_width=$(if $(MD_AD100_510_FB_LAYER2_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER2_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb2_scaling_height=$(if $(MD_AD100_510_FB_LAYER2_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER2_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo "\\" >> $(soc_fb_init_file)

	$(Q)echo -n " user_fb3_enable=$(if $(MD_AD100_510_FB_LAYER3_USER_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_width=$(if $(MD_AD100_510_FB_LAYER3_USER_ENABLE),$(MD_AD100_510_FB_LAYER3_USER_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_height=$(if $(MD_AD100_510_FB_LAYER3_USER_ENABLE),$(MD_AD100_510_FB_LAYER3_USER_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_xpos=$(if $(MD_AD100_510_FB_LAYER3_USER_ENABLE),$(MD_AD100_510_FB_LAYER3_USER_XPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_ypos=$(if $(MD_AD100_510_FB_LAYER3_USER_ENABLE),$(MD_AD100_510_FB_LAYER3_USER_YPOS),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_scaling_enable=$(if $(MD_AD100_510_FB_LAYER3_USER_SCALING_ENABLE),1,0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_scaling_width=$(if $(MD_AD100_510_FB_LAYER3_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER3_USER_SCALING_WIDTH),0)" >> $(soc_fb_init_file)
	$(Q)echo -n " user_fb3_scaling_height=$(if $(MD_AD100_510_FB_LAYER3_USER_SCALING_ENABLE),$(MD_AD100_510_FB_LAYER3_USER_SCALING_HEIGHT),0)" >> $(soc_fb_init_file)
	$(Q)echo  >> $(soc_fb_init_file)

endef
