
#-------------------------------------------------------
package_name = x2600_510_dummy_sound_card
package_depends =
package_module_src = devices/alsa-sound-card/x2600_510_dummy_sound_card/
package_make_hook =
package_init_hook =
package_finalize_hook = x2600_510_dummy_sound_card_finalize_hook
package_clean_hook =
#-------------------------------------------------------

x2600_510_dummy_sound_card_init_file = output/x2600_510_dummy_sound_card.sh

define x2600_510_dummy_sound_card_finalize_hook
	$(Q)cp devices/alsa-sound-card/x2600_510_dummy_sound_card/x2600_510_dummy_sound_card.ko output/
	$(Q)echo 'insmod x2600_510_dummy_sound_card.ko ' > $(x2600_510_dummy_sound_card_init_file)
endef