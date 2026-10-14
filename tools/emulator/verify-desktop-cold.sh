#!/bin/sh
# Prepared VM only: check packaged GUI readiness from a stopped Wine prefix.
set -eu
bb=/userdata/busybox
[ "$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)" = linux,dummy-virt ] || exit 1
package=${1:?extracted package root}; label=${2:-desktop-cold-final}
case "$label" in ''|.|..|*[!A-Za-z0-9._-]*) exit 2 ;; esac
game="$package/school-tycoon"
export DISPLAY=:95 LD_LIBRARY_PATH=/userdata/graphics/usr/lib/aarch64-linux-gnu
export WINEPREFIX=/userdata/prefix-gpu-fast BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_LD_LIBRARY_PATH="$game/runtime/x86_64:/userdata/wine-10.0-amd64-wow64/lib/wine/x86_64-unix"
export HOME=/userdata/home FONTCONFIG_FILE=/userdata/window-fonts.conf WINEDEBUG=err+all
unset LD_PRELOAD LD_AUDIT
# Refuse to interrupt any original-game session in this private guest.
if ps -eo comm | awk '/^SchoolTycoon/ {found=1} END {exit !found}'; then
    printf '%s\n' 'Stop the owned game before testing a cold desktop.' >&2; exit 2
fi
"$game/box64" /userdata/wine-10.0-amd64-wow64/bin/wineserver -k || :
work=$("$bb" mktemp -d /userdata/desktop-cold.XXXXXX)
helper_pid=
cleanup() {
    trap - EXIT HUP INT TERM
    if [ -n "$helper_pid" ]; then kill "$helper_pid" 2>/dev/null || :; wait "$helper_pid" 2>/dev/null || :; fi
    "$game/box64" /userdata/wine-10.0-amd64-wow64/bin/wineserver -k || :
    "$bb" cp "$work/stderr.txt" "/userdata/$label.stderr.txt" 2>/dev/null || :
    "$bb" cp "$work/desktop-ready.txt" "/userdata/$label.ready.txt" 2>/dev/null || :
}
trap cleanup EXIT
trap 'exit 143' HUP TERM
start=$(date +%s)
(cd "$work"; exec "$game/box64" /userdata/wine-10.0-amd64-wow64/bin/wine "$game/st-desktop-ready.exe") >"$work/stdout.txt" 2>"$work/stderr.txt" &
helper_pid=$!
while ! grep -q '^ST-DESKTOP-READY' "$work/desktop-ready.txt" 2>/dev/null; do
    if ! kill -0 "$helper_pid" 2>/dev/null || [ "$(($(date +%s)-start))" -ge 180 ]; then
        printf '%s\n' 'ST-DESKTOP-COLD-FAIL'; exit 1
    fi
    sleep 1
done
elapsed=$(($(date +%s)-start))
sleep 2
kill -0 "$helper_pid"
printf 'ST-DESKTOP-COLD-PASS: private window ready in %s seconds; helper awaits game window\n' "$elapsed"
