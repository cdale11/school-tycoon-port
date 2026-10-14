#!/bin/sh
# Read-only process and guest memory samples for a private VM test.
set -eu
bb=/userdata/busybox
model=$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)
[ "$model" = linux,dummy-virt ] || exit 1
pid=${1:?application PID required}
name=${2:?output name required}
case "$pid" in ''|*[!0-9]*) exit 2 ;; esac
case "$name" in ''|.|..|*[!A-Za-z0-9._-]*) exit 2 ;; esac
# Use process start time as well as PID so a reused PID ends this observation.
start=$("$bb" awk '{print $22}' "/proc/$pid/stat")
while [ -r "/proc/$pid/stat" ]; do
    current=$("$bb" awk '{print $22}' "/proc/$pid/stat")
    [ "$current" = "$start" ] || break
    {
        "$bb" date -u '+%Y-%m-%dT%H:%M:%SZ'
        "$bb" awk '/^(VmRSS|RssAnon|VmSwap|Threads):/' "/proc/$pid/status"
        "$bb" awk '/^(MemAvailable|SwapFree):/' /proc/meminfo
    } >>"/userdata/$name"
    "$bb" sleep 30
done
