
#-------------------------------------------------------
package_name = ad100_510_icodec_sound_card
package_depends =
package_module_src = devices/alsa-sound-card/ad100_510_icodec_sound_card/
package_make_hook =
package_init_hook =
package_finalize_hook = ad100_510_icodec_sound_card_finalize_hook
package_clean_hook =
#-------------------------------------------------------

ad100_510_icodec_sound_card_init_file = output/ad100_510_icodec_sound_card.sh

define ad100_510_icodec_sound_card_finalize_hook
	$(Q)cp devices/alsa-sound-card/ad100_510_icodec_sound_card/ad100_510_icodec_sound_card.ko output/
	$(Q)echo 'insmod ad100_510_icodec_sound_card.ko ' > $(ad100_510_icodec_sound_card_init_file)
endef