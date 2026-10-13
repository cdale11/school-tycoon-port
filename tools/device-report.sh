#!/bin/sh
# Read-only metadata collection; no package installation or game execution.
# Usage on the handheld: sh device-report.sh > /userdata/school-tycoon-device.txt
set -u

printf '%s\n' 'School Tycoon device report' '=== Architecture and kernel ==='
uname -m
uname -r
printf '\n%s\n' '=== Firmware ==='
for report_file in /etc/os-release /usr/share/batocera/batocera.version; do
    if [ -r "$report_file" ]; then
        printf '%s\n' "$report_file"
        cat "$report_file"
    fi
done
printf '\n%s\n' '=== Memory ==='
if [ -r /proc/meminfo ]; then
    awk '/^(MemTotal|MemAvailable|SwapTotal):/ { print }' /proc/meminfo
fi
printf '\n%s\n' '=== Commands available on PATH ==='
for report_cmd in box86 box64 wine wine64 wineserver glxinfo eglinfo vulkaninfo Xorg Xwayland; do
    command -v "$report_cmd" || :
done
printf '\n%s\n' '=== Graphics device nodes ==='
for report_path in /dev/dri /dev/mali /dev/mali0; do
    if [ -e "$report_path" ]; then ls -ld "$report_path"; fi
done
printf '\n%s\n' '=== Graphics libraries ==='
for report_dir in /usr/lib /usr/lib64 /lib /lib64; do
    if [ -d "$report_dir" ]; then
        find "$report_dir" -maxdepth 2 \( -name 'libEGL.so*' -o -name 'libGLES*.so*' -o -name 'libGL.so*' -o -name 'libvulkan.so*' -o -name '*mali*.so*' \) -print 2>/dev/null
    fi
done
printf '\n%s\n' '=== PortMaster runtimes and control files ==='
for report_dir in /userdata/system/.local/share/PortMaster /userdata/roms/ports/PortMaster /roms/ports/PortMaster /opt/system/Tools/PortMaster /opt/tools/PortMaster; do
    if [ -d "$report_dir" ]; then
        printf '%s\n' "$report_dir"
        find "$report_dir" -maxdepth 3 \( -name '*.squashfs' -o -name '*wine*' -o -name '*box86*' -o -name '*box64*' -o -name 'control.txt' \) -print 2>/dev/null
    fi
done
printf '\n%s\n' '=== End of report ==='
