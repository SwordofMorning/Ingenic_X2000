#!/bin/bash
# Pack the two x2000 cross toolchains required by the Darwin (5.10) product line
# into this folder. Output: mips-gcc720-glibc229.tar.xz + mips-gcc930-glibc228.tar.xz
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-/hdd/System/Hi3556v200/dandelion-linux-x86_64-ubuntu18/_Workspace_/ingenic/original_bsp/ingenic/tools/toolchains}"
for t in mips-gcc720-glibc229 mips-gcc930-glibc228; do
    out="$HERE/$t.tar.xz"
    if [ -s "$out" ]; then echo "[skip] $out already exists"; continue; fi
    echo "[pack] $t  start $(date +%H:%M:%S)"
    tar -c -C "$SRC" "$t" 2>/dev/null | xz -T0 -1 > "$out.part"
    mv "$out.part" "$out"
    echo "[pack] $t  done  $(date +%H:%M:%S)  size=$(du -h "$out" | cut -f1)"
done
echo "[pack] manifest"
{ echo "# x2000 toolchain bundle manifest  generated $(date -Iseconds)"; echo "# name  bytes  sha256  files"; 
  for t in mips-gcc720-glibc229 mips-gcc930-glibc228; do
      f="$HERE/$t.tar.xz"; [ -f "$f" ] || continue
      printf "%s  %s  %s  %s\n" "$t" "$(stat -c%s "$f")" "$(sha256sum "$f" | cut -d' ' -f1)" "$(tar tf "$f" 2>/dev/null | wc -l)"
  done; } > "$HERE/MANIFEST.sha256"
cat "$HERE/MANIFEST.sha256"
echo "[pack] ALL DONE $(date +%H:%M:%S)"
