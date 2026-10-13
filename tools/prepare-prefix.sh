#!/bin/sh
# Repair interrupted Wine setup; preserve existing prefix modules and saves.
set -eu
prefix_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
prefix_wine="$prefix_root/runtime/wine-10.0-amd64-wow64"
export WINEPREFIX="$prefix_root/prefix-wine-10.0"
export WINEARCH=win64 BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_PATH="$prefix_root:$prefix_wine/bin:/bin:/usr/bin"
export BOX64_LD_LIBRARY_PATH="$prefix_root/runtime/x86_64:$prefix_wine/lib/wine/x86_64-unix"
export WINEDLLOVERRIDES='winewayland.drv=;winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG=err+all,warn+module,trace+loaddll
unset LD_PRELOAD ST_EGL_TARGET WAYLAND_DISPLAY SDL_VIDEODRIVER
mkdir -p "$prefix_root/logs" "$prefix_root/runtime"
prefix_pending="$prefix_root/runtime/prefix-repair-pending"
prefix_verified="$prefix_root/runtime/prefix-32bit-verified"
prefix_ready=true
if [ -f "$prefix_pending" ] || [ ! -f "$prefix_verified" ]; then prefix_ready=false; fi
for prefix_dll in wow64.dll wow64cpu.dll wow64win.dll kernel32.dll kernelbase.dll ntdll.dll; do
    if [ ! -s "$WINEPREFIX/drive_c/windows/system32/$prefix_dll" ]; then prefix_ready=false; fi
done
for prefix_dll in kernel32.dll kernelbase.dll ntdll.dll cmd.exe; do
    if [ ! -s "$WINEPREFIX/drive_c/windows/syswow64/$prefix_dll" ]; then prefix_ready=false; fi
done
if [ "$prefix_ready" = true ]; then
    printf '%s\n' 'Previously verified 32-bit Wine prefix is ready.'
    exit 0
fi
if [ -z "${DISPLAY:-}" ]; then
    printf '%s\n' 'Wine prefix repair requires the private X11 session; launch School Tycoon normally.' >&2
    exit 1
fi
printf '%s\n' 'Repairing and verifying Wine prefix in private X11. See logs/prefix-setup.txt.'
exec > "$prefix_root/logs/prefix-setup.txt" 2>&1
if [ -L "$WINEPREFIX" ] && [ ! -e "$WINEPREFIX" ]; then
    printf '%s\n' 'Wine prefix has a missing-target link; apply the runtime installer update.'
    exit 1
fi
: > "$prefix_pending"
restore_prefix_modules() {
    for prefix_arch in x86_64-windows i386-windows; do
        prefix_system=system32
        if [ "$prefix_arch" = i386-windows ]; then prefix_system=syswow64; fi
        prefix_dest="$WINEPREFIX/drive_c/windows/$prefix_system"
        mkdir -p "$prefix_dest"
        for prefix_source in "$prefix_wine/lib/wine/$prefix_arch/"*.dll "$prefix_wine/lib/wine/$prefix_arch/"*.exe "$prefix_wine/lib/wine/$prefix_arch/"*.drv "$prefix_wine/lib/wine/$prefix_arch/"*.sys; do
            [ -f "$prefix_source" ] || continue
            prefix_module=${prefix_source##*/}
            if [ ! -s "$prefix_dest/$prefix_module" ]; then
                if [ -e "$prefix_dest/$prefix_module" ] || [ -L "$prefix_dest/$prefix_module" ]; then
                    prefix_backup="$prefix_dest/$prefix_module.incomplete.$(date +%s)"
                    test ! -e "$prefix_backup" && test ! -L "$prefix_backup"
                    mv "$prefix_dest/$prefix_module" "$prefix_backup"
                fi
                cp -p "$prefix_source" "$prefix_dest/$prefix_module"
                printf 'Restored missing Wine builtin: %s/%s\n' "$prefix_system" "$prefix_module"
            fi
        done
    done
}
cleanup_prefix() {
    timeout -s TERM -k 5s 15s "$prefix_root/box64" "$prefix_wine/bin/wineserver" -k || :
}
trap cleanup_prefix EXIT
trap 'exit 130' INT
trap 'exit 143' HUP TERM
restore_prefix_modules
# Existing registries already came through wineboot. Re-running full update
# invokes failing Active Setup registrations (advpack) before CPU validation.
# Only a genuinely new prefix needs the bootstrap here.
if [ ! -s "$WINEPREFIX/system.reg" ] || [ ! -s "$WINEPREFIX/user.reg" ]; then
    if timeout -s TERM -k 5s 300s "$prefix_root/box64" "$prefix_wine/bin/wine" \
        "$prefix_wine/lib/wine/x86_64-windows/wineboot.exe" -u; then
        printf '%s\n' 'Initial Wine prefix update exited successfully; verifying the runtime next.'
    else
        prefix_status=$?
        printf 'Initial Wine prefix update failed: %s. Existing prefix data is preserved.\n' "$prefix_status"
        exit "$prefix_status"
    fi
else
    printf '%s\n' 'Existing Wine registries found; checking restored runtime without repeating full update.'
fi
# Stop background setup before filling any modules left missing by its copy pass.
cleanup_prefix
restore_prefix_modules
printf '%s\n' 'Checking 32-bit Windows execution before game startup (up to 120 seconds).'
# Reuse the headless 32-bit CPU path that passed earlier on the handheld.
# GDI/GL service-window failures must not be confused with CPU-loader readiness.
if (
    unset DISPLAY
    timeout -s TERM -k 5s 120s "$prefix_root/box64" "$prefix_wine/bin/wine" \
    "$WINEPREFIX/drive_c/windows/syswow64/cmd.exe" /d /c 'echo ST-PREFIX32-PASS' \
    > "$prefix_root/logs/prefix-32.stdout.txt" 2> "$prefix_root/logs/prefix-32.txt"
); then
    prefix_status=0
else
    prefix_status=$?
fi
cat "$prefix_root/logs/prefix-32.txt" "$prefix_root/logs/prefix-32.stdout.txt"
if [ "$prefix_status" != 0 ] || ! tr -d '\r' < "$prefix_root/logs/prefix-32.stdout.txt" | grep -qx ST-PREFIX32-PASS; then
    printf '32-bit Wine verification failed: %s. The game was not launched.\n' "$prefix_status"
    # Capture runtime/prefix consistency for the loader errors above. No env dump.
    printf '%s\n' '=== Core module SHA256 inventory ===' > "$prefix_root/logs/prefix-modules.txt"
    for prefix_arch in x86_64-windows i386-windows; do
        prefix_system=system32
        if [ "$prefix_arch" = i386-windows ]; then prefix_system=syswow64; fi
        for prefix_module in ntdll.dll kernel32.dll kernelbase.dll advpack.dll advapi32.dll ucrtbase.dll ole32.dll setupapi.dll version.dll win32u.dll winex11.drv; do
            for prefix_file in "$prefix_wine/lib/wine/$prefix_arch/$prefix_module" "$WINEPREFIX/drive_c/windows/$prefix_system/$prefix_module"; do
                if [ -f "$prefix_file" ]; then
                    sha256sum "$prefix_file" >> "$prefix_root/logs/prefix-modules.txt"
                else
                    printf 'Missing: %s\n' "$prefix_file" >> "$prefix_root/logs/prefix-modules.txt"
                fi
            done
        done
    done
    exit 1
fi
: > "$prefix_verified"
rm -- "$prefix_pending"
printf '%s\n' '32-bit Wine prefix verified; continuing to the game.'
