#!/bin/sh
set -eu
bb=/userdata/busybox
model=$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)
[ "$model" = linux,dummy-virt ] || exit 1
log=${1:?owned game log prefix required}
fixture=${2:?controller Python fixture path required}
helper=${3:?ARM64 controller helper library required}
case "$log" in ''|.|..|*[!A-Za-z0-9._-]*) exit 2 ;; esac
test ! -e "/userdata/$log.status.txt"
pid=$(cat "/userdata/$log.pid.txt")
case "$pid" in ''|*[!0-9]*) exit 2 ;; esac
case "$(cat /proc/$pid/comm)" in SchoolTycoon*) ;; *) exit 2 ;; esac
kill -0 "$pid"
export DISPLAY=:95 LD_LIBRARY_PATH=/userdata/graphics/usr/lib/aarch64-linux-gnu
unset LD_PRELOAD LD_AUDIT HOTKEY
python3 "$fixture" "$helper" 5 >/userdata/fpe-game-controls.txt 2>&1 &
controls_pid=$!
while kill -0 "$pid" 2>/dev/null; do
    if ! kill -0 "$controls_pid" 2>/dev/null; then
        if wait "$controls_pid"; then
            printf '%s\n' 'Controller exit shortcut requested exit.'
            kill "$pid" 2>/dev/null || :
            break
        else
            printf '%s\n' 'Controller fixture failed; leaving the game intact.'
            exit 1
        fi
    fi
    sleep 1
done
export WINEPREFIX=/userdata/prefix-gpu-fast BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_LD_LIBRARY_PATH=/userdata/x86_64:/userdata/wine-10.0-amd64-wow64/lib/wine/x86_64-unix
/userdata/box64-opt /userdata/wine-10.0-amd64-wow64/bin/wineserver -k
printf '%s\n' 'ST-GAME-EXIT-PASS: simulated Guide/Fn + Start stopped the owned game session'
