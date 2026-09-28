include .config.in

#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os -fPIC
CFLAGS += -Iinclude/
CFLAGS += -include config.h
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

obj_dir = .objs/


# ----------------------
# 编译 libutils2.so
# ----------------------
LDFLAGS =
module_name = output/libutils2.so

src-$(APP_libutils2_os) += src/lib/os.c
src-$(APP_libutils2_array) += src/lib/array.c
src-$(APP_libutils2_data_array) += src/lib/data_array.c
src-$(APP_libutils2_listmap) += src/lib/listmap.c
src-$(APP_libutils2_event) += src/lib/event_queue.c
src-$(APP_libutils2_refer) += src/lib/refer.c
src-$(APP_libutils2_boot_time) += src/lib/boot_time.c
src-$(APP_libutils2_udp) += src/lib/udp.c
src-$(APP_libutils2_cJSON) += src/lib/cJSON.c
src-$(APP_libutils2_nalu) += src/lib/nalu_buf.c
src-$(APP_libutils2_image_format_conversion) += src/lib/image_format_conversion/bayer16_to_rgb.c
src-$(APP_libutils2_image_format_conversion) += src/lib/image_format_conversion/yuv422_to_rgb.c
src-$(APP_libutils2_image_format_conversion) += src/lib/image_format_conversion/nv12_to_rgb.c
src-$(APP_libutils2_image_format_conversion) += src/lib/image_format_conversion/raw8_to_rgb.c
src-$(APP_libutils2_image_format_conversion) += src/lib/image_format_conversion/simple_bayer16_to_nv12.c
src-$(APP_libutils2_image_format_conversion) += src/lib/image_format_conversion/cmyk_to_rgb.c
CFLAGS-$(APP_libutils2_image_format_conversion) += -I../libhardware2/include

src-$(APP_libutils2_message_queue) += src/lib/message_queue.c
LDFLAGS-$(APP_libutils2_message_queue) = -pthread
LDFLAGS-$(APP_libutils2_cJSON) += -lm

src-$(APP_libutils2_nv12_rotate) += src/lib/nv12_rotate.c

include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

libs = $(filter %.so,$(all_modules))

ifneq ($(libs),)
define install_libs
	$(Q)cp -f $(libs) $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(libs) $(FS_STAGING_DIR)/usr/lib/
	$(Q)cp -rf include/libutils2 $(FS_STAGING_DIR)/usr/include/
	@echo "  installed $(libs) include/libutils2/"
endef
define clean_install_libs
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/libutils2
	@echo "  removed $(libs) include/libutils2/"
endef
endif

install:
	$(install_libs)

clean_install:
	$(clean_install_libs)

clean:
	$(Q)rm -rf output/
	$(Q)rm -rf $(module_clean_files)

.PHONY: all clean all_targets clean_install install
