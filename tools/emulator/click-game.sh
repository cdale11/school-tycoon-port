#!/bin/sh
set -eu
[ "$#" = 2 ] || exit 2
case "$1:$2" in *[!0-9:]*|:*|*:) exit 2 ;; esac
[ "$1" -lt 800 ] && [ "$2" -lt 600 ] || exit 2
bb=/userdata/busybox
[ "$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)" = linux,dummy-virt ] || exit 1
export DISPLAY=:95 LD_LIBRARY_PATH=/userdata/graphics/usr/lib/aarch64-linux-gnu
if [ -n "${ST_VM_PACKAGE:-}" ]; then
    vm_game="$ST_VM_PACKAGE/school-tycoon"
    vm_wine="$vm_game/runtime/wine-10.0-amd64-wow64"
    vm_prefix="$vm_game/prefix-wine-10.0"
    vm_box="$vm_game/box64"
    vm_data="$vm_game/gamedata"
    vm_lib="$vm_game/runtime/x86_64"
    export HOME="$vm_game/vm-home" XDG_CACHE_HOME="$vm_game/vm-cache" FONTCONFIG_FILE="$vm_game/vm-fonts.conf"
else
    vm_wine=/userdata/wine-10.0-amd64-wow64
    vm_prefix=/userdata/prefix-gpu-fast
    vm_box=/userdata/box64-opt
    vm_data=/userdata/gamedata
    vm_lib=/userdata/x86_64
fi
export WINEPREFIX="$vm_prefix" BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_LD_LIBRARY_PATH="$vm_lib:$vm_wine/lib/wine/x86_64-unix"
if [ -z "${ST_VM_PACKAGE:-}" ]; then export HOME=/userdata/home FONTCONFIG_FILE=/userdata/window-fonts.conf; fi
unset LD_PRELOAD LD_AUDIT
printf '%s  %s\n' 4c4372be0372223864a8648aacfc488fc9ed93d45c9e364609efa11f471b698a "$vm_data/SchoolTycoon.exe" | "$bb" sha256sum -c -
probe() { "$vm_box" "$vm_wine/bin/wine" /userdata/windows-st-input-ready.exe "$@"; }
/userdata/xdotool mousemove "$1" "$2"
probe "/x$1" "/y$2"
pressed=0
release() { if [ "$pressed" = 1 ]; then /userdata/xdotool mouseup 1; fi; }
trap release EXIT
trap 'exit 143' TERM HUP
trap 'exit 130' INT
/userdata/xdotool mousedown 1
pressed=1
probe /button1
/userdata/xdotool mouseup 1
pressed=0
probe /button0
printf '%s\n' ST-CLICK-PASS
