
#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CFLAGS = -Os -fPIC
CFLAGS += -Iinclude/

obj_dir = .objs/

# ----------------------
# 编译 libspeexdsp.so
# ----------------------
LDFLAGS =
module_name = output/libspeexdsp.so

src-y += src/buffer.c
src-y += src/fftwrap.c
src-y += src/filterbank.c
src-y += src/jitter.c
src-y += src/kiss_fft.c
src-y += src/kiss_fftr.c
src-y += src/mdf.c
src-y += src/preprocess.c
src-y += src/resample.c
src-y += src/smallft.c

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
	$(Q)cp -rf include/speex/ $(FS_STAGING_DIR)/usr/include/
	@echo "  installed $(libs) include/"
endef
define clean_install_libs
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir $(libs)))
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/speex
	@echo "  removed $(libs) include/speex/"
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
