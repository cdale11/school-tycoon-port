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
if [ -n "${ST_VM_PACKAGE:-}" ]; then
    vm_game="$ST_VM_PACKAGE/school-tycoon"
    vm_wine="$vm_game/runtime/wine-10.0-amd64-wow64"
    vm_prefix="$vm_game/prefix-wine-10.0"
    vm_box="$vm_game/box64"
    vm_data="$vm_game/gamedata"
    vm_lib="$vm_game/runtime/x86_64"
else
    vm_wine=/userdata/wine-10.0-amd64-wow64
    vm_prefix=/userdata/prefix-gpu-fast
    vm_box=/userdata/box64-opt
    vm_data=/userdata/gamedata
    vm_lib=/userdata/x86_64
fi
export WINEPREFIX="$vm_prefix" BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_LD_LIBRARY_PATH="$vm_lib:$vm_wine/lib/wine/x86_64-unix"
"$vm_box" "$vm_wine/bin/wineserver" -k
printf '%s\n' 'ST-GAME-EXIT-PASS: simulated Guide/Fn + Start stopped the owned game session'
