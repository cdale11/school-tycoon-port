#!/bin/sh
# Bounded read-only telemetry. No heap tracing or changes to CPU/swap policy.
set -eu
pid=${1:?game PID}; output=${2:?log path}
case "$pid" in ''|*[!0-9]*) exit 2 ;; esac
sleep_pid=
trap 'if [ -n "$sleep_pid" ]; then kill "$sleep_pid" 2>/dev/null || :; fi; rm -f "$output.tmp"; exit 0' HUP INT TERM
printf '%s\n' 'utc,rss_kib,swap_kib,threads,available_kib,swap_free_kib' > "$output"
start=$(awk '{print $22}' "/proc/$pid/stat") || exit 0
samples=0
while [ -r "/proc/$pid/stat" ]; do
    [ "$(awk '{print $22}' "/proc/$pid/stat")" = "$start" ] || break
    row=$(awk '/^VmRSS:/ {r=$2} /^VmSwap:/ {s=$2} /^Threads:/ {t=$2} /^MemAvailable:/ {a=$2} /^SwapFree:/ {f=$2} END {printf "%s,%s,%s,%s,%s",r,s,t,a,f}' "/proc/$pid/status" /proc/meminfo) || break
    printf '%s,%s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" "$row" >> "$output"
    samples=$((samples+1))
    # 120 latest samples (one hour) plus header. Only this process writes here.
    if [ "$samples" -gt 120 ]; then
        { head -n 1 "$output"; tail -n 120 "$output"; } > "$output.tmp"
        mv "$output.tmp" "$output"
    fi
    sleep 30 & sleep_pid=$!
    wait "$sleep_pid" || :
    sleep_pid=
done
