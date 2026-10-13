#!/bin/sh
# Headless Windows CPU/runtime diagnostic. Does not launch School Tycoon.
set -eu
runtime_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
wine_dir="$runtime_root/runtime/wine-10.0-amd64-wow64"
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this diagnostic on the ARM64 handheld.' >&2
    exit 1
fi
for runtime_cmd in timeout tr grep; do
    command -v "$runtime_cmd" >/dev/null || { printf 'Missing command: %s\n' "$runtime_cmd" >&2; exit 1; }
done
test -x "$runtime_root/box64" || { printf '%s\n' 'Missing Box64 CPU diagnostic package.' >&2; exit 1; }
test -x "$wine_dir/bin/wine" || { printf '%s\n' 'Run prepare-wine.sh first.' >&2; exit 1; }
test -f "$runtime_root/runtime/x86_64/libgcc_s.so.1" || {
    printf '%s\n' 'Missing x86_64 libgcc support library; extract the updated Wine scripts package.' >&2
    exit 1
}
mkdir -p "$runtime_root/logs"
exec > "$runtime_root/logs/wine.txt" 2>&1
export WINEPREFIX="$runtime_root/prefix-wine-10.0"
export WINEARCH=win64
export WINEDLLOVERRIDES='winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG='err+all,warn+all'
export BOX64_LOG=1
export BOX64_PATH="$runtime_root:$wine_dir/bin:/bin:/usr/bin"
export BOX64_LD_LIBRARY_PATH="$runtime_root/runtime/x86_64:$wine_dir/lib/wine/x86_64-unix"
export PATH="$runtime_root:$wine_dir/bin:$PATH"
unset DISPLAY WAYLAND_DISPLAY
unset BOX64_NODYNAREC BOX64_DYNAREC BOX64_DYNAREC_BIGBLOCK BOX64_DYNAREC_STRONGMEM
cleanup_wine() {
    timeout 15s "$runtime_root/box64" "$wine_dir/bin/wineserver" -k || :
}
trap cleanup_wine EXIT HUP INT TERM
printf '%s\n' 'School Tycoon Windows runtime diagnostic' 'Headless CPU test; graphics and audio are not tested.'
timeout 30s "$runtime_root/box64" --version
timeout 30s "$runtime_root/box64" "$wine_dir/bin/wine" --version
wine_failures=0
for wine_bits in 64 32; do
    if [ "$wine_bits" = 64 ]; then wine_arch=x86_64-windows; else wine_arch=i386-windows; fi
    wine_step_log="$runtime_root/logs/wine-$wine_bits.txt"
    wine_step_stdout="$runtime_root/logs/wine-$wine_bits.stdout.txt"
    printf '\n=== Windows %s-bit command execution ===\n' "$wine_bits"
    if BOX64_LOG=0 timeout 180s "$runtime_root/box64" "$wine_dir/bin/wine" \
        "$wine_dir/lib/wine/$wine_arch/cmd.exe" /d /c "echo ST-WINE$wine_bits-PASS" \
        > "$wine_step_stdout" 2> "$wine_step_log"; then
        wine_step_status=0
    else
        wine_step_status=$?
    fi
    cat "$wine_step_log"
    printf '%s\n' '=== Command stdout ==='
    cat "$wine_step_stdout"
    printf 'Command exit status: %s\n' "$wine_step_status"
    if [ "$wine_step_status" = 0 ] && tr -d '\r' < "$wine_step_stdout" | grep -qx "ST-WINE$wine_bits-PASS"; then
        printf 'Windows %s-bit execution: PASS\n' "$wine_bits"
    else
        printf 'Windows %s-bit execution: FAIL\n' "$wine_bits"
        wine_failures=$((wine_failures + 1))
    fi
done
if [ "$wine_failures" != 0 ]; then
    printf '\n%s\n' 'Windows diagnostic failed; retain wine.txt and the individual logs.'
    exit 1
fi
printf '\n%s\n' 'Windows CPU diagnostic completed successfully.'
