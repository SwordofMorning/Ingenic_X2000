#!/bin/bash
##
 # @file scripts/prune_vendor.sh
 # @date 2026/09/28
 #
 # @brief Build the vendor/ tree from the original Ingenic SDK extraction.
 #
 # @note Scope rule (see plan v2 section 2): we only ever drop WHOLE repository
 #       sub-projects. Nothing inside a kept component is modified, so the kept
 #       content stays byte-identical to the vendor snapshot and can be diffed
 #       against later vendor drops. The single exception is doc/, where only the
 #       .Markdown/ subtree is imported (PDFs are deliberately left out).
 #
 # @note Source:  _Workspace_/ingenic/original_bsp/ingenic   (read-only)
 #       Target:  _Workspace_/ingenic/vendor
 #       Meta:    vendor/meta/{repo-manifest.xml,project_lock.tsv,files.sha256,prune_manifest.tsv}
 #
 # @note Usage: scripts/prune_vendor.sh [--verify-only]
 #         --verify-only   do not copy, only print the manifest and sizes
 #
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${SRC:-$REPO_ROOT/original_bsp/ingenic}"
DST="$REPO_ROOT/vendor"
META="$DST/meta"
VERIFY_ONLY=0
[ "${1:-}" = "--verify-only" ] && VERIFY_ONLY=1

# --- kept components (whole repo sub-projects) ------------------------------
KEEP="build
bootloader/uboot-x2000
kernel/kernel
buildroot/buildroot
buildroot/buildroot_patch
module_driver
wireless/bcm
third_party
lib2d
libhardware2
libimpf
libisp
libmedia
libutils2
libmcu
factory_test
ota_updater"

# --- dropped components (recorded for the manifest) ------------------------
DROP="kernel/kernel-x1000
kernel/kernel-x1021
kernel/kernel-x2000
bootloader/uboot-x1000
bootloader/uboot-x1021
demos
wireless/aicsemi
wireless/altobeam
wireless/espressif
wireless/hisilicon
wireless/icommsemi
wireless/luat
wireless/quectel
wireless/realtek
tools/iconfigtool
tools/toolchains
doc"

die() { echo "ERROR: $*" >&2; exit 1; }
info() { echo "==> $*"; }

[ -d "$SRC" ] || die "source SDK not found: $SRC (expects the original extraction)"

size_of() { du -sm "$1" 2>/dev/null | cut -f1 || echo 0; }

if [ "$VERIFY_ONLY" = "1" ]; then
    echo "=== KEEP (whole components) ==="
    echo "$KEEP" | while read -r c; do [ -n "$c" ] && printf "%9s MB  %s\n" "$(size_of "$SRC/$c")" "$c"; done
    printf "%9s MB  %s\n" "$(size_of "$SRC/doc/.Markdown")" "doc/.Markdown (partial: Markdown subtree only)"
    echo "=== DROP ==="
    echo "$DROP" | while read -r c; do [ -n "$c" ] && printf "%9s MB  %s\n" "$(size_of "$SRC/$c")" "$c"; done
    exit 0
fi

mkdir -p "$DST" "$META"

# --- copy kept components (nested .git dirs are removed: flattened mono-repo)
info "importing kept components from $SRC"
: > "$META/prune_manifest.tsv"
printf "status\tcomponent\tmegabytes\n" >> "$META/prune_manifest.tsv"

echo "$KEEP" | while read -r c; do
    [ -n "$c" ] || continue
    [ -d "$SRC/$c" ] || { echo "  [skip] $c (not present in source)"; continue; }
    echo "  [copy] $c"
    mkdir -p "$DST/$c"
    rsync -a --exclude='.git' "$SRC/$c/" "$DST/$c/"
    printf "keep\t%s\t%s\n" "$c" "$(size_of "$SRC/$c")" >> "$META/prune_manifest.tsv"
done

# --- partial import: vendor documentation, Markdown subtree only -----------
if [ -d "$SRC/doc/.Markdown" ]; then
    info "importing doc/.Markdown (vendor docs, Markdown form)"
    mkdir -p "$DST/doc"
    rsync -a --exclude='.git' "$SRC/doc/.Markdown/" "$DST/doc/.Markdown/"
    printf "keep-partial\t%s\t%s\n" "doc/.Markdown" "$(size_of "$SRC/doc/.Markdown")" >> "$META/prune_manifest.tsv"
fi

echo "$DROP" | while read -r c; do
    [ -n "$c" ] || continue
    printf "drop\t%s\t%s\n" "$c" "$(size_of "$SRC/$c")" >> "$META/prune_manifest.tsv"
done

# --- vendor meta: repo manifest, per-project HEAD lock, file hashes --------
if [ -f "$SRC/.repo/manifests/default.xml" ]; then
    cp -f "$SRC/.repo/manifests/default.xml" "$META/repo-manifest.xml"
    info "archived repo manifest -> meta/repo-manifest.xml"
fi

if [ -f "$SRC/.repo/project.list" ]; then
    info "recording per-project HEADs -> meta/project_lock.tsv"
    {
        echo "# repo-tool project lock (captured $(date -Iseconds))"
        echo "# path<TAB>head<TAB>date<TAB>subject"
        while read -r p; do
            [ -n "$p" ] || continue
            if [ -e "$SRC/$p/.git" ]; then
                printf "%s\t%s\t%s\t%s\n" "$p" \
                    "$(git -C "$SRC/$p" rev-parse HEAD 2>/dev/null)" \
                    "$(git -C "$SRC/$p" log -1 --format=%cd --date=short 2>/dev/null)" \
                    "$(git -C "$SRC/$p" log -1 --format=%s 2>/dev/null | cut -c1-70)"
            else
                printf "%s\t-\t-\t(not a git project)\n" "$p"
            fi
        done < "$SRC/.repo/project.list"
    } > "$META/project_lock.tsv"
fi

info "hashing the imported tree -> meta/files.sha256 (this takes a minute)"
( cd "$DST" && find . -type f -not -path './meta/*' -print0 | sort -z | xargs -0 sha256sum ) > "$META/files.sha256"

info "done"
echo "vendor size : $(du -sh "$DST" | cut -f1)"
echo "files       : $(wc -l < "$META/files.sha256")"
echo "manifest    : $META/prune_manifest.tsv"
