#!/bin/sh
# Private support bundle. Never changes firmware, services, game or prefix.
set -eu
base=${ST_REPORT_BASE:-/userdata}
work=$(mktemp -d "$base/school-tycoon-support.XXXXXX")
archive="$work.tar.gz"
mkdir -p "$work/metadata" "$work/logs" "$work/drivers"
capture() { name=$1; shift; "$@" >"$work/metadata/$name.txt" 2>&1 || :; }
capture kernel uname -a
capture libc getconf GNU_LIBC_VERSION
capture mounts cat /proc/mounts
capture memory cat /proc/meminfo
capture cpu cat /proc/cpuinfo
capture iomem cat /proc/iomem
capture modules cat /proc/modules
capture devices cat /proc/devices
capture firmware cat /etc/os-release
capture input cat /proc/bus/input/devices
capture framebuffer cat /proc/fb
capture kernel-log dmesg
capture disk df -h "$base"
capture libraries sh -c 'ls -l /lib/ld-* /lib/libc.so* /usr/lib/libEGL* /usr/lib/libGLES* /usr/lib/libmali* /usr/lib/libSDL2*'
capture graphics sh -c 'ls -l /dev/dri/* /dev/mali* /dev/fb*; for f in /sys/class/graphics/fb*/name /sys/class/graphics/fb*/virtual_size /sys/class/graphics/fb*/bits_per_pixel /sys/class/drm/card*-*/status /sys/class/drm/card*-*/modes /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do [ ! -r "$f" ] || { echo "$f"; cat "$f"; }; done'
for f in /proc/config.gz /sys/firmware/fdt; do
    [ ! -r "$f" ] || cp "$f" "$work/metadata/"
done
if [ -d /sys/firmware/devicetree/base ]; then
    tar -czf "$work/metadata/device-tree.tar.gz" -C /sys/firmware/devicetree/base . 2>"$work/metadata/device-tree-errors.txt" || :
fi
# Copy resolved driver binaries once per requested ABI name, not entire /usr/lib.
for f in /usr/lib/libmali.so.0 /usr/lib/libEGL.so.1 /usr/lib/libGLESv2.so.2 /usr/lib/libSDL2-2.0.so.0; do
    [ ! -r "$f" ] || cp -L "$f" "$work/drivers/"
done
game="$base/roms/ports/school-tycoon"
if [ -d "$game/logs" ]; then
    for f in "$game"/logs/*.txt "$game"/logs/*.previous "$game"/logs/*.csv; do
        [ -f "$f" ] || continue
        # Preserve startup and rolling logs without allowing a runaway log to
        # fill the card. The launcher already bounds its own logs.
        tail -c 2097152 "$f" >"$work/logs/${f##*/}"
    done
fi
if [ -f "$game/SHA256SUMS" ]; then
    cp "$game/SHA256SUMS" "$work/metadata/package-SHA256SUMS.txt"
fi
if [ -d "$game/runtime/graphics" ]; then
    capture graphics-hashes sh -c 'sha256sum "$1"/*.so' sh "$game/runtime/graphics"
fi
printf '%s\n' 'Private diagnostic bundle: hardware metadata, device tree, driver binaries and bounded game logs.' 'No game assets, saves, Wine prefix, SSH keys, Wi-Fi profiles or complete system configuration are collected.' >"$work/CONTENTS.txt"
tar -czf "$archive" -C "$work" .
printf '\nUpload this archive in the chat:\n%s\n' "$archive"
printf 'Uncompressed collection is retained at: %s\n' "$work"
