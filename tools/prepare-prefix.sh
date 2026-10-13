#!/bin/sh
# Complete interrupted Wine setup before loading the game's graphics bridge.
# Preserve the prefix and all existing files; restore only missing WoW64 modules.
set -eu
prefix_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
prefix_wine="$prefix_root/runtime/wine-10.0-amd64-wow64"
export WINEPREFIX="$prefix_root/prefix-wine-10.0"
export WINEARCH=win64 BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_PATH="$prefix_root:$prefix_wine/bin:/bin:/usr/bin"
export BOX64_LD_LIBRARY_PATH="$prefix_root/runtime/x86_64:$prefix_wine/lib/wine/x86_64-unix"
export WINEDLLOVERRIDES='winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG=err+all
unset LD_PRELOAD ST_EGL_TARGET WAYLAND_DISPLAY SDL_VIDEODRIVER
export WINEDLLOVERRIDES='winewayland.drv=;winemenubuilder.exe,mscoree,mshtml='

mkdir -p "$prefix_root/logs"
prefix_pending="$prefix_root/runtime/prefix-repair-pending"
prefix_ready=true
if [ -f "$prefix_pending" ]; then prefix_ready=false; fi
for prefix_dll in wow64.dll wow64cpu.dll wow64win.dll; do
    if [ ! -s "$WINEPREFIX/drive_c/windows/system32/$prefix_dll" ]; then prefix_ready=false; fi
done
if [ "$prefix_ready" = true ]; then
    printf '%s\n' 'Wine WoW64 prefix files are present.'
    exit 0
fi
if [ -z "${DISPLAY:-}" ]; then
    printf '%s\n' 'Wine prefix repair requires the private X11 session; launch School Tycoon normally.' >&2
    exit 1
fi
printf '%s\n' 'Completing interrupted Wine prefix setup in private X11. See logs/prefix-setup.txt.'

exec > "$prefix_root/logs/prefix-setup.txt" 2>&1
if [ -L "$WINEPREFIX" ] && [ ! -e "$WINEPREFIX" ]; then
    printf '%s\n' 'Wine prefix has a missing-target link; apply the runtime installer update.'
    exit 1
fi
mkdir -p "$WINEPREFIX/drive_c/windows/system32" "$prefix_root/runtime"
: > "$prefix_pending"
for prefix_dll in wow64.dll wow64cpu.dll wow64win.dll; do
    if [ ! -s "$WINEPREFIX/drive_c/windows/system32/$prefix_dll" ]; then
        test -s "$prefix_wine/lib/wine/x86_64-windows/$prefix_dll"
        # Preserve an existing file or link, including a dangling one, for recovery.
        if [ -e "$WINEPREFIX/drive_c/windows/system32/$prefix_dll" ] || [ -L "$WINEPREFIX/drive_c/windows/system32/$prefix_dll" ]; then
            prefix_backup="$WINEPREFIX/drive_c/windows/system32/$prefix_dll.incomplete.$(date +%s)"
            test ! -e "$prefix_backup" && test ! -L "$prefix_backup"
            mv "$WINEPREFIX/drive_c/windows/system32/$prefix_dll" "$prefix_backup"
        fi
        cp -p "$prefix_wine/lib/wine/x86_64-windows/$prefix_dll" "$WINEPREFIX/drive_c/windows/system32/$prefix_dll"
        printf 'Restored missing Wine builtin: %s\n' "$prefix_dll"
    fi
done
cleanup_prefix() {
    timeout -s TERM -k 5s 15s "$prefix_root/box64" "$prefix_wine/bin/wineserver" -k || :
}
trap cleanup_prefix EXIT
trap 'exit 130' INT
trap 'exit 143' HUP TERM
# Update through the 64-bit bootstrap, with GDI/X11 but no GL interposer.
if timeout -s TERM -k 5s 300s "$prefix_root/box64" "$prefix_wine/bin/wine" \
    "$prefix_wine/lib/wine/x86_64-windows/wineboot.exe" -u; then
    printf '%s\n' 'Wine prefix update exited successfully.'
else
    prefix_status=$?
    printf 'Wine prefix update failed: %s. Existing prefix data is preserved.\n' "$prefix_status"
    exit "$prefix_status"
fi
for prefix_dll in wow64.dll wow64cpu.dll wow64win.dll; do
    test -s "$WINEPREFIX/drive_c/windows/system32/$prefix_dll"
done
rm -- "$prefix_pending"
printf '%s\n' 'Wine WoW64 files restored; continuing to the game.'
