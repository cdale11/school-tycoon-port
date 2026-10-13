#!/bin/sh
# Isolated headless Weston/Xwayland + Windows GDI test. No physical display setup.
set -eu
window_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
wine_dir="$window_root/runtime/wine-10.0-amd64-wow64"
weston_dir="$window_root/runtime/weston-0.2"
weston_image=/userdata/system/.local/share/PortMaster/libs/weston_pkg_0.2.squashfs
weston_hash=0bccaeebc11296bb1fe4278c9677cb82965c5a04bcd63745fa09d78228bc1a93
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this diagnostic on the ARM64 handheld.' >&2; exit 1
fi
for window_cmd in unsquashfs sha256sum timeout date grep tr mktemp; do
    command -v "$window_cmd" >/dev/null || { printf 'Missing command: %s\n' "$window_cmd" >&2; exit 1; }
done
mkdir -p "$window_root/logs"
exec > "$window_root/logs/window.txt" 2>&1
printf '%s\n' 'School Tycoon isolated Windows window diagnostic'
test -x "$window_root/box64"
test -x "$wine_dir/bin/wine"
test -f "$window_root/windows-window-probe.exe"
# Xwayland binds filesystem sockets here, even with a private Wayland runtime.
# Preserve an existing directory and its permissions; reject other path types.
if [ -L /tmp/.X11-unix ] || { [ -e /tmp/.X11-unix ] && [ ! -d /tmp/.X11-unix ]; }; then
    printf '%s\n' '/tmp/.X11-unix is not a plain directory; leaving it untouched.'
    exit 1
fi
if [ ! -d /tmp/.X11-unix ]; then
    mkdir -m 1777 /tmp/.X11-unix
    printf '%s\n' 'Created the temporary X11 socket directory.'
fi
# Do not take over another X server or remove its lock files.
if [ -e /tmp/.X0-lock ] || [ -S /tmp/.X11-unix/X0 ]; then
    printf '%s\n' 'An X0 lock/socket already exists. Leave it intact and report this log.'
    exit 1
fi
printf '%s  %s\n' "$weston_hash" "$weston_image" | sha256sum -c -
if [ ! -d "$weston_dir" ]; then
    weston_stage=$(mktemp -d "$window_root/runtime/.weston-stage.XXXXXX")
    if unsquashfs -processors 2 -d "$weston_stage/image" "$weston_image"; then
        mv "$weston_stage/image" "$weston_dir"
        rmdir "$weston_stage"
    else
        rm -rf "$weston_stage"
        exit 1
    fi
fi
test -x "$weston_dir/wp_weston"
mkdir -p "$window_root/wayland-runtime"
chmod 700 "$window_root/wayland-runtime"
export XDG_RUNTIME_DIR="$window_root/wayland-runtime"
export XKB_CONFIG_ROOT="$weston_dir/share/xkb"
export WESTON_LIBEXEC_DIR="$weston_dir/libexec"
export WESTON_DATA_DIR="$weston_dir/data"
export WESTON_MODULE_MAP="headless-backend.so=$weston_dir/lib_aarch64/libweston-14/headless-backend.so;kiosk-shell.so=$weston_dir/lib_aarch64/weston/kiosk-shell.so;xwayland.so=$weston_dir/lib_aarch64/libweston-14/xwayland.so;"
export WESTON_HEADLESS_WIDTH=640 WESTON_HEADLESS_HEIGHT=480
export XWAYLAND_LD_LIBRARY_PATH="$weston_dir/lib_aarch64/graphics/mesa_x11_stub:$weston_dir/lib_aarch64"
export LD_LIBRARY_PATH="$XWAYLAND_LD_LIBRARY_PATH"
export PATH="$weston_dir/bin:$window_root:$wine_dir/bin:$PATH"
unset LD_PRELOAD
cat > "$window_root/runtime/window-weston.ini" <<'EOF'
[core]
idle-time=0
EOF
weston_pid=
cleanup_window() {
    if [ -n "$weston_pid" ]; then
        kill "$weston_pid" 2>/dev/null || :
        window_cleanup_start=$(date +%s)
        while kill -0 "$weston_pid" 2>/dev/null; do
            if [ "$(($(date +%s) - window_cleanup_start))" -ge 5 ]; then
                kill -KILL "$weston_pid" 2>/dev/null || :
                break
            fi
            sleep 0.2
        done
        wait "$weston_pid" 2>/dev/null || :
    fi
    BOX64_LOG=0 timeout -s TERM -k 5s 15s "$window_root/box64" "$wine_dir/bin/wineserver" -k || :
}
export WINEPREFIX="$window_root/prefix-wine-10.0"
export WINEARCH=win64 BOX64_DYNAREC=0 BOX64_LOG=0
export BOX64_PATH="$window_root:$wine_dir/bin:/bin:/usr/bin"
export BOX64_LD_LIBRARY_PATH="$window_root/runtime/x86_64:$wine_dir/lib/wine/x86_64-unix"
export WINEDLLOVERRIDES='winewayland.drv=;winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG='err+all'
trap cleanup_window EXIT HUP INT TERM
(
    cd "$weston_dir"
    exec ./wp_weston -c "$window_root/runtime/window-weston.ini" -B headless \
        --use-pixman --shell=kiosk-shell.so -i0 -Sschool-tycoon-window --xwayland
) > "$window_root/logs/window-weston.txt" 2>&1 &
weston_pid=$!
window_wait_start=$(date +%s)
while [ ! -S /tmp/.X11-unix/X0 ]; do
    if ! kill -0 "$weston_pid" 2>/dev/null || [ "$(($(date +%s) - window_wait_start))" -ge 30 ]; then
        printf '%s\n' 'Weston/Xwayland did not become ready. See window-weston.txt.'
        cat "$window_root/logs/window-weston.txt"
        exit 1
    fi
    sleep 0.2
done
export DISPLAY=:0 WAYLAND_DISPLAY=school-tycoon-window
# The probe's result is generated only in this run's dedicated log directory.
cd "$window_root/logs"
rm -f window-result.txt
window_start=$(date +%s)
if timeout -s TERM -k 5s 240s "$window_root/box64" "$wine_dir/bin/wine" \
    "$window_root/windows-window-probe.exe" > window-wine.stdout.txt 2> window-wine.txt; then
    window_status=0
else
    window_status=$?
fi
cat window-wine.txt window-wine.stdout.txt
printf 'Windows probe exit status: %s; elapsed seconds: %s\n' "$window_status" "$(($(date +%s) - window_start))"
if [ -f window-result.txt ]; then cat window-result.txt; fi
if [ "$window_status" = 0 ] && [ -f window-result.txt ] && tr -d '\r' < window-result.txt | grep -qx ST-WINDOW-PASS; then
    printf '%s\n' 'Windows X11/GDI window and pixel readback: PASS'
else
    printf '%s\n' 'Windows X11/GDI window test: FAIL'; exit 1
fi
printf '%s\n' 'This is a headless test; no window is shown on the physical screen. Direct3D 8 remains untested.'
