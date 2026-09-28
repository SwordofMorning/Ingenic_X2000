

ifneq ($(APP_ota_updater),)
define CMD_MAKE_OTA_IMG
	$(Q)make -C ../ota_updater ota_img $(MAKE_ARG)
	$(Q)rm -rf output/ota
	$(Q)cp -r ../ota_updater/ota output/
	@echo "  ota img copied --> output/ota"
endef
endif

package_name-$(APP_ota_updater) = ota_updater
include tools/package_common.mk
