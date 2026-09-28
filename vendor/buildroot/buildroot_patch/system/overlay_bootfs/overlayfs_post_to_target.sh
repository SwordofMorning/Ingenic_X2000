# overlayfs_post_to_target.sh 会被buildroot编译流程调用
# BR2_ROOTFS_POST_BUILD_SCRIPT="$(TOPDIR)/../overlayfs_post_to_target.sh"

# overlayfs_config.sh 由 Makefile 自动生成
source ../overlayfs_config.sh

target=output/target

mkdir -vp $target/usr/data/
mkdir -vp $target/rootfs
mkdir -vp $target/rootfs_ro
rm -vf $target/etc/init.d/S20urandom
rm -vf $target/etc/init.d/S40network
rm -vf $target/etc/init.d/S21mount*

cp -v ../S99overlay $target/etc/init.d/

if [ "$enable_ubi" == "y" ]; then
    cp -v ../../../rootfs_config/file/sh_utils/mount_ubifs.sh $target/bin/
    cp -v ../../../rootfs_config/file/ubi/S21mount_ubifs $target/etc/init.d/
fi

if [ "$enable_jffs2" == "y" ]; then
    cp -v ../../rootfs_config/file/sh_utils/mount_jffs2.sh $target/bin/
    cp -v ../../rootfs_config/file/jffs2/S21mount_jffs2 $target/etc/init.d/
fi
