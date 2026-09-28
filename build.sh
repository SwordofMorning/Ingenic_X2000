#!/bin/bash
##
 # @file build.sh
 # @date 2026/09/28
 #
 # @brief X2000 (Ingenic Darwin 5.10 / X2000H / SPI-NAND) SDK top-level build script.
 #
 # @note Principle 1: vendor/ is an immutable vendor baseline. We never edit files
 #       under vendor/; all product choices live in products/<product>.conf and
 #       configs/, and are overlaid into the build sandbox at sync time.
 #
 # @note Principle 2: everything compiles inside build/ (a copy of vendor/), so the
 #       repository tree stays clean and "git status" stays meaningful.
 #
 # @note Principle 3: relative paths only. Cross toolchains live in ../toolchains/
 #       (outside git) and are reached through relative symlinks, never /opt.
 #
 # @note Targets:
 #   env         Print product/build/toolchain environment
 #   toolchain   list | check | setup | clean | pack   (see toolchain_usage)
 #   sync        Copy vendor/ into build/ and overlay configs/ + toolchain links
 #   config      Run the vendor "<product>_defconfig" (text-mode configuration)
 #   all         Vendor "make all": uboot -> buildroot -> kernel -> apps -> buildroot -> images
 #   uboot|kernel|buildroot|apps
 #               Single-module passthrough (run inside build/build)
 #   fs          Rebuild the board filesystem view (buildroot) after overlay merge
 #   release     Collect flashable artifacts into build/release/<product>/
 #   check       Self-test: toolchains, product config, source tree cleanliness
 #   clean       Delete the build sandbox (never touches vendor/ or toolchains/)
 #
 # @note Environment variables:
 #   PRODUCT=darwin_v211     product config under products/
 #   JOBS=N                  parallel build jobs (default: nproc)
 #   X2000_TOOLCHAINS_DIR=   override the toolchain directory (default: <repo>/toolchains)
 #
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$REPO_ROOT/build"          # build sandbox (gitignored)
VENDOR_DIR="$REPO_ROOT/vendor"        # immutable vendor baseline (git tracked)
CONFIGS_DIR="$REPO_ROOT/configs"      # our product configs (git tracked)
PRODUCTS_DIR="$REPO_ROOT/products"    # product descriptions (git tracked)
SCRIPTS_DIR="$REPO_ROOT/scripts"
TOOLCHAINS_DIR="${X2000_TOOLCHAINS_DIR:-$REPO_ROOT/toolchains}"

PRODUCT="${PRODUCT:-darwin_v211}"
JOBS="${JOBS:-$(nproc)}"

##
 # @brief Print an error message and abort.
 #
die() { echo "ERROR: $*" >&2; exit 1; }
# End-func die

##
 # @brief Print an informational message.
 #
info() { echo "==> $*"; }
# End-func info

##
 # @brief Print usage.
 #
usage() {
    cat <<'EOF'
Usage: ./build.sh <target> [-p <product>] [-j N] [-h]

Targets:
  env                     print product / toolchain / sandbox environment
  toolchain <sub>         list | check | setup | clean | pack
  sync                    copy vendor/ into build/, overlay configs/, link toolchains
  config                  apply the product defconfig (text mode, no GUI)
  all                     full vendor build (uboot + buildroot + kernel + apps + images)
  uboot | kernel | buildroot | apps
                          build one module only
  fs                      refresh the board root filesystem view (buildroot output)
  release                 collect flashable artifacts into build/release/<product>/
  check                   self-test the whole setup
  clean                   delete the build sandbox

Options:
  -p <product>   product config name under products/ (default: darwin_v211)
  -j <N>         parallel jobs (default: nproc)
  -h             this help

Examples:
  ./build.sh toolchain check && ./build.sh toolchain setup
  ./build.sh sync && ./build.sh config && ./build.sh all
  ./build.sh release
EOF
}
# End-func usage

##
 # @brief Load products/<product>.conf into the shell environment.
 #
 # @note The conf file is plain KEY=VALUE so it can be sourced by bash and also
 #       read by scripts; it is the single source of truth for a product.
 #
load_product() {
    local f="$PRODUCTS_DIR/$PRODUCT.conf"
    [ -f "$f" ] || die "product config not found: $f"
    # shellcheck disable=SC1090
    . "$f"

    : "${PRODUCT_NAME:=$PRODUCT}"
    : "${CHIP:=x2000h}"
    : "${MEDIA:=nand}"
    : "${APP_CONFIG:=x2000_darwin_v20_5.10_nand_factory_defconfig}"
    : "${ROOTFS_CONFIG:=buildroot_x2000_510_wifi_common_defconfig}"
    : "${KERNEL_DIR:=vendor/kernel/kernel}"
    : "${KERNEL_CONFIG:=x2000_module_base_linux_sfc_nand_defconfig}"
    : "${UBOOT_CONFIG:=x2000_base_xImage_sfc_nand}"
    : "${TOOLCHAINS:=mips-gcc720-glibc229 mips-gcc930-glibc228}"
    : "${FS_OVERLAY:=fs_overlay/common}"
    : "${OTA:=n}"
    : "${OUTPUT_NAME:=$PRODUCT}"

}
# End-func load_product

##
 # @brief Print the environment.
 #
do_env() {
    load_product
    echo "repository root : $REPO_ROOT"
    echo "build sandbox   : $BUILD_DIR"
    echo "vendor tree     : $VENDOR_DIR  ($(du -sh "$VENDOR_DIR" 2>/dev/null | cut -f1 || echo 'not imported'))"
    echo "product         : $PRODUCT  ($PRODUCTS_DIR/$PRODUCT.conf)"
    echo "chip / media    : $CHIP / $MEDIA"
    echo "app defconfig   : $APP_CONFIG"
    echo "kernel          : $KERNEL_DIR  config=$KERNEL_CONFIG"
    echo "uboot config    : $UBOOT_CONFIG"
    echo "buildroot cfg   : $ROOTFS_CONFIG"
    echo "fs overlay      : $FS_OVERLAY"
    echo "OTA             : $OTA"
    echo "toolchains dir  : $TOOLCHAINS_DIR"
    echo "toolchains need : $TOOLCHAINS"
    echo "jobs            : $JOBS"
    echo "release dir     : $BUILD_DIR/release/$OUTPUT_NAME"
}
# End-func do_env

##
 # @brief Map a repository-relative vendor path to its sandbox location.
 #
 # @note vendor/kernel/kernel -> kernel/kernel : inside build/ the vendor tree
 #       sits at the sandbox root, without the "vendor/" prefix.
 #
 # @param $1 repository-relative path (e.g. vendor/kernel/kernel)
 #
sb_path() { printf '%s' "${1#vendor/}"; }
# End-func sb_path

##
 # @brief Verify the source prerequisites (product conf, vendor tree).
 #
require_vendor() {
    [ -d "$VENDOR_DIR/build" ] || die "vendor tree missing ($VENDOR_DIR). Run scripts/prune_vendor.sh first."
    [ -d "$VENDOR_DIR/kernel/kernel" ] || die "vendor kernel tree missing ($VENDOR_DIR/kernel/kernel)."
    [ -d "$TOOLCHAINS_DIR" ] || die "toolchain dir missing ($TOOLCHAINS_DIR)."
}
# End-func require_vendor

##
 # @brief Print toolchain usage.
 #
toolchain_usage() {
    cat <<'EOF'
Usage: ./build.sh toolchain <sub>

  list    show which toolchains the product needs, what is present, what is extracted
  check   verify sha256 against MANIFEST.sha256, verify gcc version, compile a smoke test
  setup   extract the toolchain archives into <repo>/toolchains/ (idempotent)
  clean   delete the extracted toolchain trees (archives are kept)
  pack    (re)create the archives from the original vendor SDK tree
EOF
}
# End-func toolchain_usage

##
 # @brief Verify one toolchain archive against the manifest and its expected gcc version.
 #
 # @param $1 toolchain name, e.g. mips-gcc720-glibc229
 # @return 0 when the archive is present and, when extracted, usable
 #
toolchain_check_one() {
    local t="$1"
    local arch="$TOOLCHAINS_DIR/$t.tar.xz"
    local dir="$TOOLCHAINS_DIR/$t"
    local want_gcc
    case "$t" in
        mips-gcc720-glibc229) want_gcc="7.2.0" ;;
        mips-gcc930-glibc228) want_gcc="9.3.0" ;;
        *)                    want_gcc="" ;;
    esac

    if [ ! -f "$arch" ]; then
        echo "  [MISS] $t.tar.xz   -> put the archive into $TOOLCHAINS_DIR/"
        return 1
    fi

    local want_sha got_sha
    want_sha="$(awk -v n="$t" '$1==n {print $3}' "$TOOLCHAINS_DIR/MANIFEST.sha256" 2>/dev/null || true)"
    got_sha="$(sha256sum "$arch" | cut -d' ' -f1)"
    if [ -n "$want_sha" ] && [ "$want_sha" != "$got_sha" ]; then
        echo "  [FAIL] $t.tar.xz  sha256 mismatch"
        echo "         expect $want_sha"
        echo "         actual $got_sha"
        return 1
    fi

    if [ ! -x "$dir/bin/mips-linux-gnu-gcc" ]; then
        echo "  [OK  ] $t.tar.xz  sha256 ok, not extracted yet -> run: ./build.sh toolchain setup"
        return 0
    fi

    local ver
    ver="$("$dir/bin/mips-linux-gnu-gcc" -dumpversion 2>/dev/null || echo '?')"
    if [ -n "$want_gcc" ] && [ "$ver" != "$want_gcc" ]; then
        echo "  [WARN] $t extracted, gcc $ver (expected $want_gcc)"
        return 1
    fi
    # smoke test: compile and link a trivial mips program
    local tmp; tmp="$(mktemp -d)"
    printf '#include <stdio.h>\nint main(void){puts("tc-ok");return 0;}\n' > "$tmp/t.c"
    if "$dir/bin/mips-linux-gnu-gcc" "$tmp/t.c" -o "$tmp/t" >/dev/null 2>&1 && file "$tmp/t" | grep -q "MIPS"; then
        echo "  [ OK ] $t  gcc $ver  smoke test passed"
        rm -rf "$tmp"; return 0
    fi
    echo "  [FAIL] $t  smoke test failed (cannot build a MIPS binary)"
    rm -rf "$tmp"; return 1
}
# End-func toolchain_check_one

##
 # @brief toolchain list / check / setup / clean.
 #
do_toolchain() {
    load_product
    local sub="${1:-list}"
    local rc=0
    case "$sub" in
        list)
            echo "product     : $PRODUCT"
            echo "needs       : $TOOLCHAINS"
            echo "directory   : $TOOLCHAINS_DIR"
            echo
            local t
            for t in $TOOLCHAINS; do
                local arch="$TOOLCHAINS_DIR/$t.tar.xz"
                local dir="$TOOLCHAINS_DIR/$t"
                printf '  %-26s archive:%-5s extracted:%-5s\n' "$t" \
                    "$([ -f "$arch" ] && echo yes || echo NO)" \
                    "$([ -d "$dir" ] && echo yes || echo NO)"
            done
            ;;
        check)
            echo "checking toolchains for product '$PRODUCT' ..."
            local t
            for t in $TOOLCHAINS; do toolchain_check_one "$t" || rc=1; done
            [ "$rc" = 0 ] && echo "toolchains: OK" || echo "toolchains: PROBLEMS FOUND (see above)"
            ;;
        setup)
            local t
            for t in $TOOLCHAINS; do
                local arch="$TOOLCHAINS_DIR/$t.tar.xz"
                local dir="$TOOLCHAINS_DIR/$t"
                [ -f "$arch" ] || { echo "  [skip] $t.tar.xz not present"; rc=1; continue; }
                if [ -d "$dir" ] && [ -x "$dir/bin/mips-linux-gnu-gcc" ]; then
                    echo "  [skip] $t already extracted"
                    continue
                fi
                echo "  [x] extracting $t ..."
                tar xf "$arch" -C "$TOOLCHAINS_DIR"
            done
            [ "$rc" = 0 ] && echo "toolchains: setup done" || echo "toolchains: incomplete (missing archives, see above)"
            ;;
        clean)
            local t
            for t in $TOOLCHAINS; do
                [ -d "$TOOLCHAINS_DIR/$t" ] && rm -rf "$TOOLCHAINS_DIR/$t" && echo "  removed $t"
            done
            ;;
        pack)
            if [ -x "$TOOLCHAINS_DIR/pack_toolchains.sh" ]; then
                SRC="${SRC:-}" "$TOOLCHAINS_DIR/pack_toolchains.sh"
            else
                die "pack_toolchains.sh not found in $TOOLCHAINS_DIR"
            fi
            ;;
        *)
            toolchain_usage; exit 1
            ;;
    esac
    exit "$rc"
}
# End-func do_toolchain

##
 # @brief Copy vendor/ into the sandbox, overlay our configs, link the toolchains.
 #
 # @note No --delete: build artifacts in the sandbox are intentionally kept so
 #       incremental builds work. Use "clean" to start over.
 #
sync_sources() {
    require_vendor
    load_product

    command -v rsync >/dev/null || die "rsync not found"

    info "syncing vendor tree -> $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    rsync -a --exclude='/.git' "$VENDOR_DIR/" "$BUILD_DIR/"

    # The vendor framework copies finished images into build/build/output/.
    # Only its "all" target creates that directory, so we create it here for the
    # single-module targets as well; a stray file with the same name (seen once
    # after an aborted run) would make later cp calls fail, so clean it up.
    if [ -e "$BUILD_DIR/build/output" ] && [ ! -d "$BUILD_DIR/build/output" ]; then
        info "removing stray file build/build/output (leftover of an aborted run)"
        rm -f "$BUILD_DIR/build/output"
    fi
    mkdir -p "$BUILD_DIR/build/output"

    # our product configs take precedence over the vendor ones.
    # configs/build/ mirrors the layout of vendor/build/ (framework files such as
    # Config.in, and optionally configs/<name>_defconfig).
    if [ -d "$CONFIGS_DIR/build" ]; then
        info "overlaying configs/build -> build/build"
        mkdir -p "$BUILD_DIR/build"
        rsync -a "$CONFIGS_DIR/build/" "$BUILD_DIR/build/"
    fi
    if [ -d "$CONFIGS_DIR/kernel" ]; then
        info "overlaying configs/kernel -> $(sb_path "$KERNEL_DIR")/arch/mips/configs"
        mkdir -p "$BUILD_DIR/$(sb_path "$KERNEL_DIR")/arch/mips/configs"
        rsync -a "$CONFIGS_DIR/kernel/" "$BUILD_DIR/$(sb_path "$KERNEL_DIR")/arch/mips/configs/"
    fi
    # u-boot board table (e.g. the LPJ calibration for this board revision).
    # Our copy differs from the vendor file only in the lines we deliberately change.
    if [ -d "$CONFIGS_DIR/uboot" ]; then
        info "overlaying configs/uboot -> bootloader/uboot-x2000"
        for f in "$CONFIGS_DIR/uboot"/*; do
            [ -f "$f" ] || continue
            local base; base="$(basename "$f")"
            case "$base" in
                boards.cfg) cp -f "$f" "$BUILD_DIR/bootloader/uboot-x2000/boards.cfg" ;;
                *)          echo "  [warn] unhandled uboot overlay file: $base" ;;
            esac
        done
    fi

    link_toolchains
    info "sync done"
}
# End-func sync_sources

##
 # @brief Make the vendor-expected toolchain paths resolve inside the sandbox.
 #
 # @note The vendor Makefiles reference the toolchains through paths relative to
 #       the SDK root and to buildroot/, so we only need relative symlinks here.
 #       Nothing is copied and nothing outside the repository is used.
 #
link_toolchains() {
    load_product
    local linkdir="$BUILD_DIR/tools/toolchains"
    mkdir -p "$linkdir"
    local t
    for t in $TOOLCHAINS; do
        local dir="$TOOLCHAINS_DIR/$t"
        if [ ! -d "$dir" ]; then
            echo "  [warn] toolchain not extracted: $t (run: ./build.sh toolchain setup)"
            continue
        fi
        ln -sfn "$(realpath --relative-to="$linkdir" "$dir")" "$linkdir/$t"
    done
}
# End-func link_toolchains

##
 # @brief Run the vendor defconfig for the product (text mode configuration).
 #
do_config() {
    sync_sources
    load_product
    info "applying product defconfig: $APP_CONFIG"
    ( cd "$BUILD_DIR/build" && make "$APP_CONFIG" )
    info "configuration written: build/build/.config.in + build/build/config.h"
}
# End-func do_config

##
 # @brief Run a vendor make target inside the sandbox (no sync).
 #
 # @param $1 vendor make target
 #
vendor_make_nosync() {
    local target="$1"
    ( cd "$BUILD_DIR/build" && make "$target" THREAD_ARG="-j$JOBS" )
}
# End-func vendor_make_nosync

##
 # @brief Pass a vendor make target through inside the sandbox.
 #
 # @param $1 vendor make target (all, uboot, kernel, buildroot, apps, ...)
 #
do_vendor_make() {
    local target="$1"
    sync_sources
    info "make $target (jobs=$JOBS)"
    vendor_make_nosync "$target"
}
# End-func do_vendor_make

##
 # @brief Pre-build packages that later packages need at compile time.
 #
 # @note The vendor package order in build/product.mk builds third_party/speexdsp
 #       AFTER libmedia, but libmedia's speex AEC devices include <speex/...>
 #       headers that have to be installed into the buildroot staging sysroot
 #       first. Building that one package early fixes the order without touching
 #       any vendor file. Extend this list if a future configuration introduces
 #       more such dependencies.
 #
prebuild_staging_deps() {
    local cfg="$BUILD_DIR/build/.config.in"
    [ -f "$cfg" ] || return 0
    if grep -q '^APP_speexdsp=y' "$cfg"; then
        info "pre-building third_party/speexdsp (required by libmedia speex AEC)"
        ( cd "$BUILD_DIR/build" && make app_third_party/speexdsp THREAD_ARG="-j$JOBS" ) \
            || die "third_party/speexdsp pre-build failed"
    fi
}
# End-func prebuild_staging_deps

##
 # @brief Merge the product filesystem overlay into the buildroot target tree.
 #
 # @note Applied AFTER the vendor rootfs assembly (rootfs_config injection and
 #       the vendor post-build script), so our files win. The rootfs image is
 #       regenerated by the following buildroot pass, which is why do_all calls
 #       this between "vendor make all" and a second "buildroot" run.
 #
fs_overlay_apply() {
    load_product
    local target_dir="$BUILD_DIR/buildroot/buildroot/output/target"
    [ -d "$target_dir" ] || return 0
    local ov n
    for ov in $FS_OVERLAY; do
        local d="$REPO_ROOT/$ov"
        if [ -d "$d" ]; then
            n="$(find "$d" -type f | wc -l)"
            info "fs overlay: $ov ($n files)"
            rsync -a "$d/" "$target_dir/"
        else
            echo "  [warn] overlay dir missing: $d"
        fi
    done
}
# End-func fs_overlay_apply

##
 # @brief Full build: sync, prepare, vendor "make all", overlay, repack rootfs.
 #
 # @note Phase 1 builds u-boot and buildroot (the latter creates the host
 #       toolchain and the staging sysroot), phase 2 pre-builds packages whose
 #       headers later packages need, phase 3 runs the vendor "all" (kernel +
 #       apps + rootfs + images), phase 4 merges our filesystem overlay and
 #       regenerates the root filesystem image.
 #
do_all() {
    sync_sources
    info "phase 1/4: u-boot"
    vendor_make_nosync uboot
    info "phase 1/4: buildroot (host toolchain + staging sysroot)"
    vendor_make_nosync buildroot
    info "phase 2/4: staging dependencies"
    prebuild_staging_deps
    info "phase 3/4: vendor make all (kernel + apps + rootfs + images)"
    vendor_make_nosync all
    info "phase 4/4: product filesystem overlay + rootfs repack"
    fs_overlay_apply
    vendor_make_nosync buildroot
    if [ -f "$BUILD_DIR/buildroot/buildroot/output/images/rootfs.squashfs" ]; then
        cp -f "$BUILD_DIR/buildroot/buildroot/output/images/rootfs.squashfs" "$BUILD_DIR/build/output/"
        info "refreshed build/build/output/rootfs.squashfs with the overlay applied"
    fi
}
# End-func do_all

##
 # @brief Rebuild the board root filesystem with the product overlay applied.
 #
do_fs() {
    sync_sources
    load_product
    local target_dir="$BUILD_DIR/buildroot/buildroot/output/target"
    [ -d "$target_dir" ] || die "buildroot target dir not found ($target_dir); run ./build.sh all first"
    fs_overlay_apply
    vendor_make_nosync buildroot
    if [ -f "$BUILD_DIR/buildroot/buildroot/output/images/rootfs.squashfs" ]; then
        cp -f "$BUILD_DIR/buildroot/buildroot/output/images/rootfs.squashfs" "$BUILD_DIR/build/output/"
        info "build/build/output/rootfs.squashfs refreshed"
    fi
}
# End-func do_fs

##
 # @brief Collect flashable artifacts into build/release/<product>/.
 #
do_release() {
    sync_sources
    load_product
    local out="$BUILD_DIR/build/output"
    local rel="$BUILD_DIR/release/$OUTPUT_NAME"
    [ -d "$out" ] || die "vendor output dir not found ($out); build first (./build.sh all)"
    mkdir -p "$rel"

    # file name == partition name (Hi3556 house style)
    local f
    for f in u-boot-spl-pad.bin u-boot-with-spl.bin xImage xImage_split rootfs.squashfs \
             rootfs.ubifs userdata.ubifs overlay_bootfs.squashfs image.bin; do
        [ -f "$out/$f" ] && cp -f "$out/$f" "$rel/"
    done
    if [ -d "$out/ota" ] && [ "$OTA" = "y" ]; then
        info "copying OTA package"
        rsync -a --delete "$out/ota/" "$rel/ota/"
    fi

    ( cd "$rel" && md5sum -- $(ls -1 | grep -v -E '^(md5sum.txt|notes.txt|NAND_LAYOUT.md)$') > md5sum.txt )

    {
        echo "product     : $PRODUCT_NAME ($PRODUCT)"
        echo "chip/media  : $CHIP / $MEDIA"
        echo "app defconfig: $APP_CONFIG"
        echo "kernel      : $KERNEL_DIR ($KERNEL_CONFIG)"
        echo "uboot       : $UBOOT_CONFIG"
        echo "build time  : $(date -Iseconds)"
        echo "git         : $(git -C "$REPO_ROOT" describe --always --dirty 2>/dev/null || echo '-')"
        echo "layout      : see device/NAND_LAYOUT.md"
    } > "$rel/notes.txt"

    info "release ready: $rel"
    ls -lh "$rel"
}
# End-func do_release

##
 # @brief Self-test: product conf, vendor tree, toolchains, sandbox, git cleanliness.
 #
do_check() {
    local rc=0
    echo "--- product ---"
    if [ -f "$PRODUCTS_DIR/$PRODUCT.conf" ]; then
        load_product
        echo "  [ OK ] $PRODUCT / chip=$CHIP media=$MEDIA"
        echo "         apps=$APP_CONFIG"
        echo "         kernel=$KERNEL_CONFIG uboot=$UBOOT_CONFIG buildroot=$ROOTFS_CONFIG"
    else
        echo "  [FAIL] missing $PRODUCTS_DIR/$PRODUCT.conf"; rc=1
    fi

    echo "--- vendor tree ---"
    local v="$VENDOR_DIR"
    for d in build bootloader/uboot-x2000 kernel/kernel buildroot/buildroot buildroot/buildroot_patch \
             module_driver wireless/bcm third_party doc/.Markdown; do
        if [ -d "$v/$d" ]; then echo "  [ OK ] vendor/$d"; else echo "  [MISS] vendor/$d"; rc=1; fi
    done

    echo "--- toolchains ---"
    for t in $TOOLCHAINS; do toolchain_check_one "$t" || rc=1; done

    echo "--- host ---"
    command -v python2 >/dev/null && echo "  [ OK ] python2 $(python2 -V 2>&1 | cut -d' ' -f2)" || echo "  [warn] python2 missing (vendor build may need it)"
    command -v rsync   >/dev/null && echo "  [ OK ] rsync"  || { echo "  [FAIL] rsync"; rc=1; }
    command -v git-lfs >/dev/null && echo "  [ OK ] git-lfs" || echo "  [warn] git-lfs missing (LFS files will stay as pointers)"

    echo "--- repository ---"
    if [ -d "$REPO_ROOT/.git" ]; then
        local dirty; dirty="$(git -C "$REPO_ROOT" status --porcelain | wc -l)"
        [ "$dirty" = "0" ] && echo "  [ OK ] git status clean" || echo "  [warn] $dirty changed/untracked entries"
    else
        echo "  [warn] not a git repository yet"
    fi

    echo
    [ "$rc" = 0 ] && echo "check: PASS" || echo "check: FAIL (see above)"
    exit "$rc"
}
# End-func do_check

##
 # @brief Delete the build sandbox.
 #
 # @param $1 "all" also removes vendor-side leftovers (none expected: we never build in-tree)
 #
do_clean() {
    local what="${1:-}"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        info "build sandbox deleted: $BUILD_DIR"
    else
        info "no build sandbox to delete"
    fi
    if [ "$what" = "all" ]; then
        info "vendor tree untouched by design (builds happen only in the sandbox)"
    fi
}
# End-func do_clean

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
TARGET=""
while [ $# -gt 0 ]; do
    case "$1" in
        -p) PRODUCT="$2"; shift 2 ;;
        -j) JOBS="$2";    shift 2 ;;
        -h|--help) usage; exit 0 ;;
        -*) die "unknown option: $1" ;;
        *)  if [ -z "$TARGET" ]; then TARGET="$1"; shift; else TARGET_ARGS="${TARGET_ARGS:-} $1"; shift; fi ;;
    esac
done

[ -n "$TARGET" ] || { usage; exit 1; }

case "$TARGET" in
    env)        do_env ;;
    toolchain)  do_toolchain ${TARGET_ARGS:-list} ;;
    sync)       sync_sources ;;
    config)     do_config ;;
    all)        do_all ;;
    uboot)      do_vendor_make uboot ;;
    kernel)     do_vendor_make kernel ;;
    buildroot)  do_vendor_make buildroot ;;
    apps)       do_vendor_make apps ;;
    fs)         do_fs ;;
    release)    do_release ;;
    check)      do_check ;;
    clean)      do_clean ${TARGET_ARGS:-} ;;
    *)          die "unknown target: $TARGET (see ./build.sh -h)" ;;
esac
