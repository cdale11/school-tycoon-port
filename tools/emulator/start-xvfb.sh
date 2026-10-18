#!/bin/sh
# Prepared cloud VM only: this overlays /usr/bin to provide Xorg's xkbcomp.
set -eu
bb=/userdata/busybox
model=$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)
if [ "$model" != linux,dummy-virt ]; then
    printf '%s\n' 'This script requires the private QEMU virt guest.' >&2
    exit 1
fi
vm_display=${1:-:95}
vm_geometry=${2:-800x600x24}
case "$vm_geometry" in 800x600x24|720x480x24) ;; *) exit 2 ;; esac
vm_number=${vm_display#:}
case "$vm_number" in ''|*[!0-9]*) exit 2 ;; esac
[ "$vm_display" = ":$vm_number" ] || exit 2
if [ -e "/tmp/.X$vm_number-lock" ] || [ -e "/tmp/.X11-unix/X$vm_number" ]; then
    printf 'Display %s already has a lock or socket; leaving it intact.\n' "$vm_display" >&2
    exit 1
fi
"$bb" mkdir -p /userdata/original-bin /userdata/vm-bin
if ! "$bb" awk '$2 == "/userdata/original-bin" { found=1 } END { exit !found }' /proc/self/mounts; then
    "$bb" mount --bind /usr/bin /userdata/original-bin
fi
if ! "$bb" awk '$2 == "/usr/bin" { found=1 } END { exit !found }' /proc/self/mounts; then
    # Resolve symlinks in their original location before overlaying /usr/bin.
    # Firmware coreutils links use ../libexec, which breaks when relocated.
    for entry in /userdata/original-bin/*; do
        name=${entry##*/}
        target=$("$bb" readlink -f "/usr/bin/$name") || continue
        case "$target" in /usr/bin/*) target="/userdata/original-bin/${target#/usr/bin/}" ;; esac
        "$bb" ln -sfn "$target" "/userdata/vm-bin/$name"
    done
    "$bb" rm -f /userdata/vm-bin/xkbcomp
    "$bb" cp /userdata/graphics/usr/bin/xkbcomp /userdata/vm-bin/xkbcomp
    "$bb" mount --bind /userdata/vm-bin /usr/bin
fi
unset LD_PRELOAD
export LD_LIBRARY_PATH=/userdata/graphics/usr/lib/aarch64-linux-gnu
export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export LIBGL_DRIVERS_PATH=/userdata/graphics/usr/lib/aarch64-linux-gnu/dri
export HOME=/userdata/home XDG_CACHE_HOME=/userdata/cache
"$bb" mkdir -p "$HOME" "$XDG_CACHE_HOME"
exec /userdata/graphics/usr/bin/Xvfb "$vm_display" -screen 0 "$vm_geometry" \
    -xkbdir /userdata/graphics/usr/share/X11/xkb -nolisten tcp -ac
