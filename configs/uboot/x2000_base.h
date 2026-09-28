#ifndef __X2000_BASE_H__
#define	__X2000_BASE_H__

/* VOT product: writable root filesystem.
 * rootfs is UBIFS inside a UBI volume on the "rootfs" MTD partition,
 * mounted read-write (vendor default: read-only squashfs on mtdblock).
 * The device string and the rw flag are set in x2000_base_common.h. */
#define CONFIG_ROOTFS_UBI
#define CONFIG_ROOTFS2_UBI
#define CONFIG_ARG_QUIET
#define CONFIG_SPL_SERIAL_SUPPORT

#include "x2000_base_common.h"

#endif /* __X2000_BASE_H__ */
