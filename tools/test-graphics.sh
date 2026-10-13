#!/bin/sh
# Native off-screen rendering plus read-only display-runtime inventory.
set -eu
graphics_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this diagnostic on the ARM64 handheld.' >&2
    exit 1
fi
mkdir -p "$graphics_root/logs"
exec > "$graphics_root/logs/graphics.txt" 2>&1
printf '%s\n' 'School Tycoon native graphics diagnostic' '=== System graphics libraries ==='
for graphics_dir in /usr/lib /usr/lib64 /lib /lib64; do
    if [ -d "$graphics_dir" ]; then
        find "$graphics_dir" -maxdepth 2 \( -name 'libEGL.so*' -o -name 'libGLESv2.so*' -o -name 'libGL.so*' -o -name 'libSDL2*.so*' -o -name 'libgbm.so*' \) -print 2>/dev/null || :
    fi
done
printf '\n%s\n' '=== System display commands ==='
for graphics_cmd in weston Xwayland Xorg unsquashfs; do command -v "$graphics_cmd" || :; done
printf '\n%s\n' '=== Weston runtime ==='
for graphics_file in /userdata/system/.local/share/PortMaster/libs/weston_pkg_0.2.squashfs /userdata/roms/ports/PortMaster/libs/weston_pkg_0.2.squashfs; do
    if [ -r "$graphics_file" ]; then
        ls -l "$graphics_file"
        if command -v sha256sum >/dev/null; then sha256sum "$graphics_file"; fi
        if command -v unsquashfs >/dev/null; then
            unsquashfs -ll "$graphics_file" | awk '/wp_weston|westonwrap|Xwayland|libX11\.so|libGL\.so|libEGL\.so/ {print}'
        fi
    fi
done
printf '\n%s\n' '=== Native EGL/GLES rendering ==='
graphics_start=$(date +%s)
if timeout -s TERM -k 5s 30s "$graphics_root/native-egl-probe.aarch64"; then
    graphics_status=0
else
    graphics_status=$?
fi
printf 'Native probe exit status: %s; elapsed seconds: %s\n' "$graphics_status" "$(($(date +%s) - graphics_start))"
printf '%s\n' 'This tests a native pbuffer, not Wine, GLX, Direct3D 8, or a visible game window.'
exit "$graphics_status"
