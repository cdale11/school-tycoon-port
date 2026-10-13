#!/bin/sh
# KNULLI Ports entry. Original assets stay on the user's device.
set -eu
ports_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
game_root="$ports_dir/school-tycoon"
mkdir -p "$game_root/logs" "$game_root/runtime"
exec > "$game_root/logs/launcher.txt" 2>&1
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'School Tycoon requires the ARM64 KNULLI handheld.'; exit 1
fi
# Reuse large, previously installed runtimes without moving or altering them.
for runtime_name in wine-10.0-amd64-wow64 weston-0.2; do
    if [ ! -e "$game_root/runtime/$runtime_name" ] && [ -d "$ports_dir/school-tycoon-runtime-probe/runtime/$runtime_name" ]; then
        ln -s "$ports_dir/school-tycoon-runtime-probe/runtime/$runtime_name" "$game_root/runtime/$runtime_name"
    fi
done
if [ ! -f "$game_root/gamedata/SchoolTycoon.exe" ]; then
    printf '%s\n' 'Copy the contents of your ST folder into ports/school-tycoon/gamedata/.'
    exit 1
fi
# Reuse an existing prefix to avoid another first-run installation on this device.
if [ ! -e "$game_root/prefix-wine-10.0" ] && [ -d "$ports_dir/school-tycoon-runtime-probe/prefix-wine-10.0" ]; then
    ln -s "$ports_dir/school-tycoon-runtime-probe/prefix-wine-10.0" "$game_root/prefix-wine-10.0"
fi
sh "$game_root/prepare-wine.sh"
if [ -f /tmp/gamecontrollerdb.txt ]; then
    export SDL_GAMECONTROLLERCONFIG_FILE=/tmp/gamecontrollerdb.txt
fi
# Verified CPU baseline. Optional dynarec remains unverified on this device.
if [ -f "$game_root/cpu-mode.txt" ]; then
    ST_CPU_MODE=$(cat "$game_root/cpu-mode.txt")
    export ST_CPU_MODE
fi
exec sh "$game_root/run-game.sh" game
