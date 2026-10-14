#!/bin/sh
# KNULLI Ports entry. Original assets stay on the user's device.
set -eu
ports_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
game_root="$ports_dir/school-tycoon"
mkdir -p "$game_root/logs" "$game_root/runtime"
# Retain one preceding attempt; rotate only launcher-owned log files.
for log_name in launcher.txt game.txt game-weston.txt controls.txt; do
    if [ -f "$game_root/logs/$log_name" ]; then
        mv "$game_root/logs/$log_name" "$game_root/logs/$log_name.previous"
    fi
done
# Retain bounded tails of the previous game's Wine output before the drainers open.
for log_name in game-wine.txt game-wine.stdout.txt; do
    if [ -f "$game_root/logs/$log_name" ]; then
        tail -c 262144 "$game_root/logs/$log_name" > "$game_root/logs/$log_name.previous"
    fi
done
exec > "$game_root/logs/launcher.txt" 2>&1
printf 'Started: %s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
if [ -f "$game_root/debug.txt" ] && [ "$(cat "$game_root/debug.txt")" = 1 ]; then
    export ST_DEBUG=1
fi
{
    uname -a
    getconf GNU_LIBC_VERSION || :
    cat /etc/os-release 2>/dev/null || :
    awk '/^(MemTotal|MemAvailable|SwapTotal|SwapFree|CmaTotal|CmaFree):/' /proc/meminfo
    df -Pk "$game_root"
    if [ -f "$game_root/gamedata/SchoolTycoon.exe" ]; then sha256sum "$game_root/gamedata/SchoolTycoon.exe"; fi
    printf 'Runtime build: '; cat "$game_root/BUILD-ID.txt" 2>/dev/null || :
    printf 'Debug: %s; CPU request: %s\n' "${ST_DEBUG:-0}" "${ST_CPU_MODE:-interpreter}"
    ls -l /dev/mali0 /usr/lib/libEGL.so.1 /usr/lib/libGLESv2.so.2 /usr/lib/libSDL2-2.0.so.0 2>/dev/null || :
} > "$game_root/logs/system.txt" 2>&1
if [ -f "$game_root/SHA256SUMS" ]; then
    (cd "$ports_dir"; sha256sum -c "$game_root/SHA256SUMS") || {
        printf '%s\n' 'Package verification failed. Re-extract the complete package before launching.'
        exit 1
    }
fi
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'School Tycoon requires the ARM64 KNULLI handheld.'; exit 1
fi
# Reuse large, previously installed runtimes without moving or altering them.
for runtime_name in wine-10.0-amd64-wow64 weston-0.2; do
    if [ -L "$game_root/runtime/$runtime_name" ] && [ ! -e "$game_root/runtime/$runtime_name" ]; then
        printf 'Removing missing-target runtime link: %s\n' "$runtime_name"
        rm -- "$game_root/runtime/$runtime_name"
    fi
    if [ ! -e "$game_root/runtime/$runtime_name" ] && [ -d "$ports_dir/school-tycoon-runtime-probe/runtime/$runtime_name" ]; then
        ln -s "$ports_dir/school-tycoon-runtime-probe/runtime/$runtime_name" "$game_root/runtime/$runtime_name"
    fi
done
if [ ! -f "$game_root/gamedata/SchoolTycoon.exe" ]; then
    printf '%s\n' 'Copy the contents of your ST folder into ports/school-tycoon/gamedata/.'
    exit 1
fi
# Preserve a dangling prefix link for recovery; create a new prefix if its
# old probe target was removed. Never delete a real prefix or saved data.
if [ -L "$game_root/prefix-wine-10.0" ] && [ ! -e "$game_root/prefix-wine-10.0" ]; then
    prefix_backup="$game_root/prefix-wine-10.0.missing-target.$(date +%s)"
    if [ -e "$prefix_backup" ] || [ -L "$prefix_backup" ]; then
        printf '%s\n' 'Prefix backup path exists; leaving the missing-target link untouched.'; exit 1
    fi
    mv "$game_root/prefix-wine-10.0" "$prefix_backup"
    printf 'Preserved missing-target Wine prefix link: %s\n' "$prefix_backup"
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
