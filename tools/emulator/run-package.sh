#!/bin/sh
# Test exact archive binaries with the prepared guest's software graphics fixture.
# This substitutes displays/drivers; it does not certify the handheld Mali path.
set -eu
bb=/userdata/busybox
[ "$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)" = linux,dummy-virt ] || exit 1
package=${1:?extracted package root}
log=${2:-game-final-package}
case "$log" in ''|.|..|*[!A-Za-z0-9._-]*) exit 2 ;; esac
(cd "$package"; sha256sum -c school-tycoon/SHA256SUMS)
game="$package/school-tycoon"
export PATH=/bin:/usr/bin:/sbin:/usr/sbin
export HOME=/userdata/home XDG_CACHE_HOME=/userdata/cache
. /userdata/host/emulator/shared/graphics-env.sh
export DISPLAY=:95 ST_SDL_DISPLAY=:96 GALLIUM_DRIVER=softpipe
export FONTCONFIG_FILE=/userdata/window-fonts.conf
export ST_EGL_TARGET=SchoolTycoon.exe WINEDEBUG=err+all
export WINEPREFIX=/userdata/prefix-gpu-fast BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_LD_LIBRARY_PATH="$game/runtime/x86_64:/userdata/wine-10.0-amd64-wow64/lib/wine/x86_64-unix"
export BOX64_PATH=/userdata/wine-10.0-amd64-wow64/bin:/usr/bin:/bin
preload="/userdata/libsdl-display.so:$game/runtime/graphics/libst-gl-present.so:$game/runtime/graphics/libst-glx-config.so:$game/runtime/graphics/libst-egl-prime.so:$ST_CRUSTY_LIBRARY"
unset LD_PRELOAD LD_AUDIT WINESERVER
pipes=$("$bb" mktemp -d /userdata/st-package-pipes.XXXXXX)
app_pid=;telemetry_pid=;stdout_pid=;stderr_pid=;desktop_pid=
cleanup() {
    trap - EXIT HUP INT TERM
    if [ -n "$desktop_pid" ]; then kill "$desktop_pid" 2>/dev/null || :; wait "$desktop_pid" 2>/dev/null || :; fi
    if [ -n "$app_pid" ]; then kill "$app_pid" 2>/dev/null || :; fi
    if [ -n "$telemetry_pid" ]; then kill "$telemetry_pid" 2>/dev/null || :; wait "$telemetry_pid" 2>/dev/null || :; fi
    for pid in "$stdout_pid" "$stderr_pid"; do
        if [ -n "$pid" ]; then kill "$pid" 2>/dev/null || :; wait "$pid" 2>/dev/null || :; fi
    done
    "$game/box64" /userdata/wine-10.0-amd64-wow64/bin/wineserver -k || :
    "$bb" rm -f "$pipes/stdout" "$pipes/stderr" "$pipes/desktop-ready.txt"; "$bb" rmdir "$pipes"
}
trap cleanup EXIT
trap 'exit 143' HUP TERM
trap 'exit 130' INT
# Match production's temporary desktop readiness helper before applying GL hooks.
(cd "$pipes"; exec "$game/box64" /userdata/wine-10.0-amd64-wow64/bin/wine "$game/st-desktop-ready.exe") >"/userdata/$log.desktop.txt" 2>&1 &
desktop_pid=$!
desktop_start=$(date +%s)
while ! grep -q '^ST-DESKTOP-READY' "$pipes/desktop-ready.txt" 2>/dev/null; do
    if ! kill -0 "$desktop_pid" 2>/dev/null || [ "$(($(date +%s)-desktop_start))" -ge 180 ]; then
        printf '%s\n' 'VM package desktop readiness failed'; exit 1
    fi
    sleep 1
done
printf '%s\n' 'ST-PACKAGE-DESKTOP-READY'
"$bb" mkfifo "$pipes/stdout" "$pipes/stderr"
"$game/st-log" "/userdata/$log.stdout.txt" <"$pipes/stdout" & stdout_pid=$!
"$game/st-log" "/userdata/$log.stderr.txt" <"$pipes/stderr" & stderr_pid=$!
"$bb" rm -f "/userdata/$log.status.txt"
cd /userdata/gamedata
LD_PRELOAD="$preload" "$game/box64" /userdata/wine-10.0-amd64-wow64/bin/wine ./SchoolTycoon.exe >"$pipes/stdout" 2>"$pipes/stderr" &
app_pid=$!
printf '%s\n' "$app_pid" >"/userdata/$log.pid.txt"
sh "$game/sample-session.sh" "$app_pid" "/userdata/$log.memory.csv" & telemetry_pid=$!
if wait "$app_pid"; then status=0; else status=$?; fi
app_pid=
printf '%s\n' "$status" >"/userdata/$log.status.txt"
exit "$status"
