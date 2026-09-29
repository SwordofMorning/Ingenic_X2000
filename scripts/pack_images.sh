#!/bin/sh
##
 # @file scripts/pack_images.sh
 # @date 2026/09/28
 #
 # @brief Pack the root filesystem images from the buildroot target tree.
 #
 # @note This script MUST run inside fakeroot (build.sh calls it that way).
 #       Reason: buildroot applies its users table and normalises ownership
 #       while generating the images - our wrapper bypasses that step when it
 #       packs the images itself (needed because the vendor post-build script
 #       runs inside buildroot's target-finalize and would drop our overlay
 #       files). Consequences of packing without fakeroot were real:
 #         - the sshd/dbus accounts never made it into the image, so sshd
 #           exited with "Privilege separation user sshd does not exist";
 #         - every file ended up owned by the build user's uid (1000), which
 #           on the device is a different account.
 #
 # @note Ownership rules that daemons enforce:
 #         /var/empty       must be root-owned (openssh aborts otherwise)
 #         /var/run/dbus    must belong to the dbus user (socket directory)
 #       buildroot's users table chowns the daemon home directories to the
 #       daemon user, which is wrong for /var/empty, so it is corrected here.
 #
 # @note Inputs (environment):
 #         BR_DIR              buildroot directory (contains .config and output/)
 #         JOBS                parallelism for mksquashfs
 #         ROOT_PASSWORD_HASH  crypt(3) SHA-512 hash of the factory root password
 #                             (build.sh computes it from products/<product>.conf);
 #                             installed as field 2 of the root line in /etc/shadow
 #
set -e

BR="${BR_DIR:?BR_DIR not set}"
JOBS="${JOBS:-4}"
TARGET="$BR/output/target"
IMAGES="$BR/output/images"
CONFIG="$BR/.config"
HOST="$BR/output/host"
USERS_TABLE="$BR/output/build/buildroot-fs/full_users_table.txt"

log() { echo "  [pack] $*"; }

[ -d "$TARGET" ] || { echo "ERROR: target dir missing: $TARGET" >&2; exit 1; }
[ -f "$CONFIG" ] || { echo "ERROR: buildroot .config missing: $CONFIG" >&2; exit 1; }

cfg() { sed -n "s/^$1=//p" "$CONFIG" | head -1; }

# ---------------------------------------------------------------------------
# 1) ownership: everything root, then the buildroot users table, then the few
#    exceptions that daemons require.
# ---------------------------------------------------------------------------
log "normalising ownership to root:root"
chown -h -R 0:0 "$TARGET"

if [ -f "$USERS_TABLE" ]; then
    log "applying users table ($(grep -c . "$USERS_TABLE") entries: $(awk '{print $1}' "$USERS_TABLE" | tr '\n' ' '))"
    BR2_CONFIG="$CONFIG" "$BR/support/scripts/mkusers" "$USERS_TABLE" "$TARGET"

    # openssh refuses to run when its privilege separation directory is not
    # root-owned (sshd.c: "%s must be owned by root and not group or world-writable")
    if [ -d "$TARGET/var/empty" ]; then
        chown -h 0:0 "$TARGET/var/empty"
        chmod 755 "$TARGET/var/empty"
    fi

    # dbus needs its socket directory owned by the dbus account
    dbus_uid="$(awk -F: '$1=="dbus"{print $3}' "$TARGET/etc/passwd" 2>/dev/null)"
    dbus_gid="$(awk -F: '$1=="dbus"{print $4}' "$TARGET/etc/passwd" 2>/dev/null)"
    if [ -n "$dbus_uid" ] && [ -d "$TARGET/var/run/dbus" ]; then
        chown -h -R "$dbus_uid:$dbus_gid" "$TARGET/var/run/dbus"
        log "var/run/dbus -> uid $dbus_uid"
    fi
fi

# ---------------------------------------------------------------------------
# 2) strip, following buildroot's own target-finalize policy
#    buildroot strips the target tree during its final pass, and the full
#    "make all" flow runs that pass after the vendor packages. When we repack
#    after a LATER package build (./build.sh module_driver, make apps), the
#    freshly installed libraries and executables are still unstripped - that
#    inflated the rootfs by about 5 MB once (libmedia.so, libavpu.so, the demo
#    and test binaries). Reproducing the policy here makes the image
#    independent of the build order.
#    Policy (see buildroot/Makefile, STRIP_FIND_CMD):
#      - strip executables (mode +x) and shared objects
#      - never strip kernel modules (*.ko)
#      - ld-*.so* and libpthread*.so* get debug symbols only
# ---------------------------------------------------------------------------
stripcmd=""
# Kconfig strings are quoted ("mips-linux-gnu"), so strip the quotes; fall back to
# any *-strip present in the buildroot host directory.
prefix="$(cfg BR2_TOOLCHAIN_EXTERNAL_PREFIX | tr -d '"')"
for c in "$HOST/bin/$prefix-strip" "$HOST/bin/mips-linux-gnu-strip" "$HOST/bin"/*-strip; do
    [ -x "$c" ] && { stripcmd="$c"; break; }
done

if [ "$(cfg BR2_STRIP_strip)" = "y" ] && [ -x "$stripcmd" ] && command -v file >/dev/null 2>&1; then
    log "stripping binaries (executables + shared libraries, keeping *.ko intact)"
    find "$TARGET" -type f \( -perm /111 -o -name '*.so*' \) -not -name '*.ko' -print0 \
        | xargs -0 -r file 2>/dev/null \
        | grep -E ': *ELF ' | cut -d: -f1 \
        | while IFS= read -r f; do
              case "$f" in
                  */ld-*.so*|*/libpthread*.so*)
                      "$stripcmd" --strip-debug "$f" 2>/dev/null || true ;;
                  *)
                      "$stripcmd" -s "$f" 2>/dev/null || true ;;
              esac
          done
else
    log "WARNING: not stripping (strip=$stripcmd, BR2_STRIP_strip=$(cfg BR2_STRIP_strip), file=$(command -v file || echo missing))"
fi

# ---------------------------------------------------------------------------
# 3) factory root password: products/<product>.conf -> /etc/shadow field 2
#    Must run after mkusers (which owns the account list) and only ever touches
#    the root line, so the daemon accounts keep their locked passwords.
#    Mode 0600 root:root: only the daemons and login/passwd (all root) read it.
# ---------------------------------------------------------------------------
if [ -n "${ROOT_PASSWORD_HASH:-}" ]; then
    SHADOW="$TARGET/etc/shadow"
    if [ -f "$SHADOW" ]; then
        tmp="${SHADOW}.new"
        awk -F: -v h="$ROOT_PASSWORD_HASH" 'BEGIN { OFS = ":" } $1 == "root" { $2 = h } { print }' \
            "$SHADOW" > "$tmp"
        grep -q '^root:' "$tmp" || printf 'root:%s:::::::\n' "$ROOT_PASSWORD_HASH" >> "$tmp"
        cat "$tmp" > "$SHADOW"      # keep the inode (hard links, if any)
        rm -f "$tmp"
        chown -h 0:0 "$SHADOW"
        chmod 600 "$SHADOW"
        log "root password installed (shadow, mode 600)"
    else
        echo "  [warn] no /etc/shadow in the target tree: root password not installed" >&2
    fi
fi

# ---------------------------------------------------------------------------
# 4) rootfs.squashfs (read-only fallback image)
# ---------------------------------------------------------------------------
if [ "$(cfg BR2_TARGET_ROOTFS_SQUASHFS)" = "y" ] && [ -x "$HOST/bin/mksquashfs" ]; then
    comp="gzip"
    [ "$(cfg BR2_TARGET_ROOTFS_SQUASHFS4_LZ4)" = "y" ]  && comp="lz4"
    [ "$(cfg BR2_TARGET_ROOTFS_SQUASHFS4_LZO)" = "y" ]  && comp="lzo"
    [ "$(cfg BR2_TARGET_ROOTFS_SQUASHFS4_LZMA)" = "y" ] && comp="lzma"
    [ "$(cfg BR2_TARGET_ROOTFS_SQUASHFS4_XZ)" = "y" ]   && comp="xz"
    [ "$(cfg BR2_TARGET_ROOTFS_SQUASHFS4_ZSTD)" = "y" ] && comp="zstd"
    log "rootfs.squashfs (-comp $comp)"
    "$HOST/bin/mksquashfs" "$TARGET" "$IMAGES/rootfs.squashfs" \
        -noappend -processors "$JOBS" -comp "$comp" > /dev/null
fi

# ---------------------------------------------------------------------------
# 5) rootfs.ubifs + rootfs.ubi (the writable root filesystem)
# ---------------------------------------------------------------------------
if [ "$(cfg BR2_TARGET_ROOTFS_UBIFS)" = "y" ] && [ -x "$HOST/sbin/mkfs.ubifs" ]; then
    leb="$(cfg BR2_TARGET_ROOTFS_UBIFS_LEBSIZE)"
    minio="$(cfg BR2_TARGET_ROOTFS_UBIFS_MINIOSIZE)"
    maxleb="$(cfg BR2_TARGET_ROOTFS_UBIFS_MAXLEBCNT)"
    ucomp="zlib"
    [ "$(cfg BR2_TARGET_ROOTFS_UBIFS_RT_LZO)" = "y" ] && ucomp="lzo"
    log "rootfs.ubifs (leb=$leb minio=$minio maxleb=$maxleb comp=$ucomp)"
    "$HOST/sbin/mkfs.ubifs" -d "$TARGET" -e "$leb" -c "$maxleb" -m "$minio" \
        -x "$ucomp" -o "$IMAGES/rootfs.ubifs" > /dev/null

    if [ "$(cfg BR2_TARGET_ROOTFS_UBI)" = "y" ] && [ -x "$HOST/sbin/ubinize" ]; then
        peb="$(cfg BR2_TARGET_ROOTFS_UBI_PEBSIZE)"
        ucfg="${BR_DIR}/../ubinize.cfg"
        {
            echo "[ubifs]"
            echo "mode=ubi"
            echo "vol_id=0"
            echo "vol_type=dynamic"
            echo "vol_name=rootfs"
            echo "vol_alignment=1"
            echo "vol_flags=autoresize"
            echo "image=$IMAGES/rootfs.ubifs"
        } > "$ucfg"
        log "rootfs.ubi (peb=$peb)"
        "$HOST/sbin/ubinize" -o "$IMAGES/rootfs.ubi" -m "$minio" -p "$peb" "$ucfg" > /dev/null
        rm -f "$ucfg"
    fi
fi

log "done: $(ls -1 "$IMAGES" | tr '\n' ' ')"
