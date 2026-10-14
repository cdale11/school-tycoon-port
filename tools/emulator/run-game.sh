#!/bin/sh
# Original-game test in the prepared private VM, independent of shell history.
set -eu
bb=/userdata/busybox
model=$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)
[ "$model" = linux,dummy-virt ] || exit 1
export PATH=/bin:/usr/bin:/sbin:/usr/sbin
export HOME=/userdata/home XDG_CACHE_HOME=/userdata/cache
"$bb" mkdir -p "$HOME" "$XDG_CACHE_HOME"
script_dir=$("$bb" dirname "$0")
. "$script_dir/graphics-env.sh"
export DISPLAY=:95 ST_SDL_DISPLAY=:96
export FONTCONFIG_FILE=/userdata/window-fonts.conf
export ST_EGL_TARGET=SchoolTycoon.exe
export WINEPREFIX=/userdata/prefix-gpu-fast BOX64_DYNAREC=0 BOX64_LOG=1
export BOX64_LD_LIBRARY_PATH=/userdata/x86_64:/userdata/wine-10.0-amd64-wow64/lib/wine/x86_64-unix
export BOX64_PATH=/userdata/wine-10.0-amd64-wow64/bin:/usr/bin:/bin
unset WINESERVER
app_preload="/userdata/libsdl-display.so:$LD_PRELOAD"
app_audit=
if [ "${ST_VM_ALLOC_AUDIT:-0}" = 1 ]; then
    test "${ST_VM_ALLOC_TRACE:-0}" != 1
    test -f /userdata/libst-alloc-audit.so
    app_audit=/userdata/libst-alloc-audit.so
fi
if [ "${ST_VM_ALLOC_TRACE:-0}" = 1 ]; then
    test -f /userdata/libst-alloc-trace.so
    app_preload="/userdata/libst-alloc-trace.so:$app_preload"
fi
unset LD_PRELOAD LD_AUDIT
test -x /userdata/box64-opt
test -f /userdata/libst-gl-present.so
test -f /userdata/libsdl-display.so
test -f /userdata/gamedata/SchoolTycoon.exe
cd /userdata/gamedata
log=${ST_VM_LOG_PREFIX:-game-verified-env}
case "$log" in ''|.|..|*[!A-Za-z0-9._-]*) exit 2 ;; esac
"$bb" rm -f "/userdata/$log.status.txt"
if LD_PRELOAD="$app_preload" LD_AUDIT="$app_audit" /userdata/box64-opt \
    /userdata/wine-10.0-amd64-wow64/bin/wine ./SchoolTycoon.exe \
    >"/userdata/$log.stdout.txt" 2>"/userdata/$log.stderr.txt"; then
    status=0
else
    status=$?
fi
printf '%s\n' "$status" >"/userdata/$log.status.txt"
exit "$status"
