#!/bin/sh
# Headless Windows CPU/runtime diagnostic. Does not launch School Tycoon.
set -eu
runtime_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
wine_dir="$runtime_root/runtime/wine-10.0-amd64-wow64"
wine_mode=${1:-default}
case "$wine_mode" in
    default|strongmem|interpreter) ;;
    *) printf '%s\n' 'Usage: sh test-wine.sh [default|strongmem|interpreter]' >&2; exit 2 ;;
esac
wine_log_base=wine
if [ "$wine_mode" != default ]; then wine_log_base="wine-$wine_mode"; fi
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this diagnostic on the ARM64 handheld.' >&2
    exit 1
fi
for runtime_cmd in timeout tr grep date awk; do
    command -v "$runtime_cmd" >/dev/null || { printf 'Missing command: %s\n' "$runtime_cmd" >&2; exit 1; }
done
test -x "$runtime_root/box64" || { printf '%s\n' 'Missing Box64 CPU diagnostic package.' >&2; exit 1; }
test -x "$wine_dir/bin/wine" || { printf '%s\n' 'Run prepare-wine.sh first.' >&2; exit 1; }
test -f "$runtime_root/runtime/x86_64/libgcc_s.so.1" || {
    printf '%s\n' 'Missing x86_64 libgcc support library; extract the updated Wine scripts package.' >&2
    exit 1
}
mkdir -p "$runtime_root/logs"
exec > "$runtime_root/logs/$wine_log_base.txt" 2>&1
export WINEPREFIX="$runtime_root/prefix-wine-10.0"
export WINEARCH=win64
export WINEDLLOVERRIDES='winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG='err+all'
export BOX64_LOG=1
export BOX64_PATH="$runtime_root:$wine_dir/bin:/bin:/usr/bin"
export BOX64_LD_LIBRARY_PATH="$runtime_root/runtime/x86_64:$wine_dir/lib/wine/x86_64-unix"
export PATH="$runtime_root:$wine_dir/bin:$PATH"
unset DISPLAY WAYLAND_DISPLAY
unset BOX64_NODYNAREC BOX64_DYNAREC BOX64_DYNAREC_BIGBLOCK BOX64_DYNAREC_STRONGMEM
case "$wine_mode" in
    strongmem) export BOX64_DYNAREC=1 BOX64_DYNAREC_STRONGMEM=2 ;;
    interpreter) export BOX64_DYNAREC=0 ;;
esac
cleanup_wine() {
    timeout -s TERM -k 5s 15s "$runtime_root/box64" "$wine_dir/bin/wineserver" -k || :
}
trap cleanup_wine EXIT HUP INT TERM
printf '%s\n' 'School Tycoon Windows runtime diagnostic' 'Headless CPU test; graphics and audio are not tested.'
printf 'Mode: %s\n' "$wine_mode"
report_memory() {
    awk '/^(MemTotal|MemAvailable|SwapTotal|SwapFree):/ {print}' /proc/meminfo
}
report_memory
timeout 30s "$runtime_root/box64" --version
timeout 30s "$runtime_root/box64" "$wine_dir/bin/wine" --version
wine_failures=0
for wine_bits in 64 32; do
    if [ "$wine_bits" = 64 ]; then wine_arch=x86_64-windows; else wine_arch=i386-windows; fi
    wine_step_log="$runtime_root/logs/$wine_log_base-$wine_bits.txt"
    wine_step_stdout="$runtime_root/logs/$wine_log_base-$wine_bits.stdout.txt"
    printf '\n=== Windows %s-bit command execution ===\n' "$wine_bits"
    wine_step_start=$(date +%s)
    if BOX64_LOG=0 timeout -s TERM -k 5s 120s "$runtime_root/box64" "$wine_dir/bin/wine" \
        "$wine_dir/lib/wine/$wine_arch/cmd.exe" /d /c "echo ST-WINE$wine_bits-PASS" \
        > "$wine_step_stdout" 2> "$wine_step_log"; then
        wine_step_status=0
    else
        wine_step_status=$?
    fi
    wine_step_elapsed=$(($(date +%s) - wine_step_start))
    cat "$wine_step_log"
    printf '%s\n' '=== Command stdout ==='
    cat "$wine_step_stdout"
    printf 'Command exit status: %s\n' "$wine_step_status"
    printf 'Elapsed seconds: %s (timeout: 120 seconds, kill grace: 5 seconds)\n' "$wine_step_elapsed"
    report_memory
    wine_marker_seen=0
    if tr -d '\r' < "$wine_step_stdout" | grep -qx "ST-WINE$wine_bits-PASS"; then
        wine_marker_seen=1
        printf 'Windows %s-bit marker printed: YES\n' "$wine_bits"
    fi
    if [ "$wine_step_status" = 0 ] && [ "$wine_marker_seen" = 1 ]; then
        printf 'Windows %s-bit execution: PASS\n' "$wine_bits"
    else
        printf 'Windows %s-bit execution: FAIL\n' "$wine_bits"
        if [ "$wine_marker_seen" = 1 ]; then
            printf '%s\n' 'Windows code executed, but command shutdown did not complete successfully.'
        fi
        wine_failures=$((wine_failures + 1))
    fi
    cleanup_wine
done
printf '\n%s\n' '=== Available kernel OOM reports (may include older events) ==='
if command -v dmesg >/dev/null; then
    dmesg 2>/dev/null | grep -E 'Out of memory|oom-kill|Killed process' | tail -15 || :
fi
if [ "$wine_failures" != 0 ]; then
    printf '\nWindows diagnostic failed; retain %s.txt and the individual logs.\n' "$wine_log_base"
    exit 1
fi
printf '\n%s\n' 'Windows CPU diagnostic completed successfully.'
