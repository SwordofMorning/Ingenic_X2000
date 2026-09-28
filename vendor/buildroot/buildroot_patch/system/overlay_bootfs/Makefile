
# 由 make config_in=~/xxx/linux/build/.config 传入
sinclude $(config_in)

export V
export Q
export CROSS_COMPILE
export FS_TARGET_DIR
export FS_STAGING_DIR

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
else
Q =
endif

defconfig=configs/buildroot_uclibc_gcc8.4_xburst_defconfig
target=tmp_buildroot/output/target

define set_ubi_config_$(APP_system_overlay_bootfs_ubi)
	$(Q)echo enable_ubi=y >> overlayfs_config.sh
endef

define set_jffs2_config_$(APP_system_overlay_bootfs_jffs2)
	$(Q)echo enable_jffs2=y >> overlayfs_config.sh
endef

all: tmp_buildroot
	$(Q)echo > overlayfs_config.sh
	$(set_ubi_config_y)
	$(set_jffs2_config_y)
	$(Q)cp $(defconfig) tmp_buildroot/.config
	$(Q)echo building overlay bootfs, if error see build.log
	$(Q)make -C tmp_buildroot > build.log 2>&1

tmp_buildroot:
	$(Q)git clone ../../../buildroot tmp_buildroot
	$(Q)wget ftp://ftp.ingenic.com.cn/sz_ingenic/linux-5.17.15.tar.xz
	$(Q)mkdir -p tmp_buildroot/dl/linux/
	$(Q)mv linux-5.17.15.tar.xz tmp_buildroot/dl/linux/

clean:
	$(Q)rm -rf tmp_buildroot
	$(Q)rm overlayfs_config.sh

clean_install:

install:

.PHONY: all clean install
