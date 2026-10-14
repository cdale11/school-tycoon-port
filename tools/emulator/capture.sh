#!/bin/sh
# Capture directly to the private output share without pausing guest storage.
set -eu
bb=/userdata/busybox
model=$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)
[ "$model" = linux,dummy-virt ] || exit 1
vm_display=${1:-:96}
name=${2:-frame.ppm}
case "$name" in ''|.|..|*[!A-Za-z0-9._-]*) exit 2 ;; esac
if ! "$bb" awk '$1 == "results" && $2 == "/userdata/results" && $3 == "9p" { found=1 } END { exit !found }' /proc/self/mounts; then
    printf '%s\n' 'Start the VM with --results-dir before capturing.' >&2
    exit 1
fi
unset LD_PRELOAD
export LD_LIBRARY_PATH=/userdata/graphics/usr/lib/aarch64-linux-gnu
export DISPLAY="$vm_display"
/userdata/capture "/userdata/results/$name"
printf 'Captured %s\n' "$name"
