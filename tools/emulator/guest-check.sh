#!/bin/bash
# Version/facts check for the private VM; never run this on the handheld.
set -u
export PATH=/bin:/usr/bin:/sbin:/usr/sbin
BB=/userdata/busybox
printf '\n=== School Tycoon ARM64 guest facts ===\n'
uname -a
getconf GNU_LIBC_VERSION
getconf PAGESIZE
cat /proc/cpuinfo
cat /proc/meminfo
"$BB" truncate -s 510201856 /userdata/swapfile
chmod 600 /userdata/swapfile
"$BB" mkswap /userdata/swapfile
"$BB" swapon /userdata/swapfile
cat /proc/swaps
HOST=/userdata/host
BOX64=/userdata/box64
WINE=/userdata/wine-10.0-amd64-wow64
export BOX64_DYNAREC=0 BOX64_LOG=1
export BOX64_LD_LIBRARY_PATH="$WINE/lib/wine/x86_64-unix:/userdata/x86_64"
export BOX64_PATH="$WINE/bin:/usr/bin:/bin"
export WINEPREFIX=/userdata/prefix
export WINEDLLOVERRIDES='winewayland.drv=;iexplore.exe=;winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG=err+all,warn+module
printf '\n=== Actual ARM64 Box64 / Wine version ===\n'
"$BOX64" --version
"$BOX64" "$WINE/bin/wine" --version
printf 'WINE-VERSION-STATUS=%s\n' "$?"
printf '\n=== Filesystem and tool readiness ===\n'
ls -l "$HOST/game/ST/SchoolTycoon.exe"
printf '\nGuest ready for Wine prefix and game tests.\n'
