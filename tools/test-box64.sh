#!/bin/sh
# Run from an extracted school-tycoon-runtime-probe directory on the handheld.
# This tests CPU translation only; it does not launch the game or change firmware.
set -eu
probe_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this diagnostic on the ARM64 handheld.' >&2
    exit 1
fi
mkdir -p "$probe_root/logs"
probe_log="$probe_root/logs/box64.txt"
exec > "$probe_log" 2>&1
printf '%s\n' 'School Tycoon CPU runtime diagnostic' '=== Firmware architecture ==='
uname -m
uname -r
getconf GNU_LIBC_VERSION
printf '%s\n' '=== Box64 version ==='
"$probe_root/box64" --version
printf '%s\n' '=== x86_64 execution ==='
# Keep inherited tuning settings from changing the diagnostic's behavior.
unset BOX64_NODYNAREC BOX64_DYNAREC BOX64_DYNAREC_BIGBLOCK BOX64_DYNAREC_STRONGMEM
BOX64_LOG=1 "$probe_root/box64" "$probe_root/x86-probe"
printf '%s\n' 'CPU diagnostic completed successfully.'
