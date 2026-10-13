#!/bin/sh
# Isolated Weston/Xwayland + GDI or D3D8 test. D3D8 may present through SDL/Mali.
set -eu
window_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
wine_dir="$window_root/runtime/wine-10.0-amd64-wow64"
weston_dir="$window_root/runtime/weston-0.2"
weston_image=/userdata/system/.local/share/PortMaster/libs/weston_pkg_0.2.squashfs
weston_hash=0bccaeebc11296bb1fe4278c9677cb82965c5a04bcd63745fa09d78228bc1a93
window_mode=${1:-gdi}
case "$window_mode" in
    game) window_log=game; window_exe=gamedata/SchoolTycoon.exe; window_result=; window_marker= ;;
    gdi) window_log=window; window_exe=windows-window-probe.exe; window_result=window-result.txt; window_marker=ST-WINDOW-PASS ;;
    d3d8|core4es)
        window_log=d3d8; window_exe=windows-d3d8-probe.exe
        if [ "$window_mode" = core4es ]; then window_log=d3d8-core4es; fi
        window_result=d3d8-result.txt; window_marker=ST-D3D8-PASS ;;
    *) printf '%s\n' 'Usage: sh test-window.sh [gdi|d3d8|core4es|game]' >&2; exit 2 ;;
esac
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this diagnostic on the ARM64 handheld.' >&2; exit 1
fi
for window_cmd in unsquashfs sha256sum timeout date grep tr mktemp; do
    command -v "$window_cmd" >/dev/null || { printf 'Missing command: %s\n' "$window_cmd" >&2; exit 1; }
done
mkdir -p "$window_root/logs"
exec > "$window_root/logs/$window_log.txt" 2>&1
printf '%s\n' 'School Tycoon isolated Windows window diagnostic'
test -x "$window_root/box64"
test -x "$wine_dir/bin/wine"
test -f "$window_root/$window_exe"
printf 'Mode: %s\n' "$window_mode"
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
export WESTON_HEADLESS_WIDTH=800 WESTON_HEADLESS_HEIGHT=600
export XWAYLAND_LD_LIBRARY_PATH="$weston_dir/lib_aarch64/graphics/mesa_x11_stub:$weston_dir/lib_aarch64"
export LD_LIBRARY_PATH="$XWAYLAND_LD_LIBRARY_PATH"
export PATH="$weston_dir/bin:$window_root:$wine_dir/bin:$PATH"
mkdir -p "$window_root/font-cache"
cat > "$window_root/runtime/window-fonts.conf" <<EOF
<?xml version="1.0"?>
<fontconfig>
  <dir>$wine_dir/share/wine/fonts</dir>
  <dir>$weston_dir/fonts</dir>
  <cachedir>$window_root/font-cache</cachedir>
</fontconfig>
EOF
export FONTCONFIG_FILE="$window_root/runtime/window-fonts.conf"
unset LD_PRELOAD
cat > "$window_root/runtime/window-weston.ini" <<'EOF'
[core]
idle-time=0
EOF
weston_pid=
controls_pid=
game_pid=
cleanup_window() {
    trap - EXIT HUP INT TERM
    if [ -n "$controls_pid" ]; then kill "$controls_pid" 2>/dev/null || :; wait "$controls_pid" 2>/dev/null || :; fi
    if [ -n "$game_pid" ]; then kill "$game_pid" 2>/dev/null || :; fi
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
export WINEDLLOVERRIDES='winewayland.drv=;iexplore.exe=;winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG='err+all'
trap cleanup_window EXIT
trap 'exit 130' INT
trap 'exit 143' HUP TERM
(
    cd "$weston_dir"
    exec ./wp_weston -c "$window_root/runtime/window-weston.ini" -B headless \
        --use-pixman --shell=kiosk-shell.so -i0 -Sschool-tycoon-window --xwayland
) > "$window_root/logs/$window_log-weston.txt" 2>&1 &
weston_pid=$!
window_wait_start=$(date +%s)
while [ ! -S /tmp/.X11-unix/X0 ]; do
    if ! kill -0 "$weston_pid" 2>/dev/null || [ "$(($(date +%s) - window_wait_start))" -ge 30 ]; then
        printf 'Weston/Xwayland did not become ready. See %s-weston.txt.\n' "$window_log"
        cat "$window_root/logs/$window_log-weston.txt"
        exit 1
    fi
    sleep 0.2
done
export DISPLAY=:0 WAYLAND_DISPLAY=school-tycoon-window
# Repair Wine/RpcSs in the verified private X11 session, before Crusty.
if [ "$window_mode" = game ]; then
    sh "$window_root/prepare-prefix.sh"
fi
window_app_libpath="$LD_LIBRARY_PATH"
window_app_preload=
if [ "$window_mode" != gdi ]; then
    window_gl=gl4es_glxpass
    if [ "$window_mode" = core4es ]; then window_gl=core4es_glxpass; fi
    window_app_libpath="$weston_dir/lib_aarch64/graphics/$window_gl:$weston_dir/lib_aarch64/graphics/crusty_glx:$weston_dir/lib_aarch64"
    test -f "$window_root/runtime/graphics/libst-egl-prime.so"
    window_app_preload="$window_root/runtime/graphics/libst-egl-prime.so:$weston_dir/lib_aarch64/graphics/crusty_glx/libcrusty.so"
    if [ "$window_mode" = game ]; then
        export ST_EGL_TARGET=SchoolTycoon.exe
    else
        export ST_EGL_TARGET=windows-d3d8-probe.exe
    fi
    export CRUSTY_LIBEGL=/usr/lib/libEGL.so.1
    export CRUSTY_LIBSDL=/usr/lib/libSDL2-2.0.so.0
    export CRUSTY_GL4ES=1 CRUSTY_WLMODE=0 CRUSTY_WLDISP=school-tycoon-window
    export SDL_VIDEODRIVER=mali
    # Avoid auto-selected GLES 3.2 and ambiguous library resolution in SDL.
    # These are native ARM libraries, never x86 copies from Wine.
    export CRUSTY_GLES=20 SDL_OPENGL_ES_DRIVER=1
    export SDL_VIDEO_EGL_DRIVER=/usr/lib/libEGL.so.1
    export SDL_VIDEO_GL_DRIVER=/usr/lib/libGLESv2.so.2
    export LIBGL_EGL=/usr/lib/libEGL.so.1 LIBGL_GLES=/usr/lib/libGLESv2.so.2
    export LIBGL_ES=2 CRUSTY_SHOW_CURSOR=1
    export WINEDEBUG='err+all,warn+d3d8,warn+wined3d'
fi
# Full game runs without a diagnostic timeout, from its asset directory.
if [ "$window_mode" = game ]; then
    case "${ST_CPU_MODE:-interpreter}" in
        interpreter) export BOX64_DYNAREC=0 ;;
        dynarec)
            export BOX64_DYNAREC=1 BOX64_DYNAREC_BIGBLOCK=0
            export BOX64_DYNAREC_FORWARD=0 BOX64_DYNAREC_STRONGMEM=2
            export BOX64_DYNAREC_SAFEFLAGS=2 BOX64_DYNAREC_CALLRET=0 ;;
        *) printf '%s\n' 'Invalid ST_CPU_MODE'; exit 2 ;;
    esac
    printf 'Launching SchoolTycoon.exe; CPU mode: %s\n' "${ST_CPU_MODE:-interpreter}"
    printf '%s\n' 'Graphics configuration is experimental; this is not a verified playable port.'
    export SDL_GAMECONTROLLERCONFIG_FILE="${SDL_GAMECONTROLLERCONFIG_FILE:-/userdata/system/.local/share/PortMaster/knulli/gamecontrollerdb.txt}"
    LD_LIBRARY_PATH="$weston_dir/lib_aarch64" "$window_root/st-controls" > "$window_root/logs/controls.txt" 2>&1 &
    controls_pid=$!
    cd "$window_root/gamedata"
    env LD_PRELOAD="$window_app_preload" LD_LIBRARY_PATH="$window_app_libpath" \
        "$window_root/box64" "$wine_dir/bin/wine" ./SchoolTycoon.exe \
        > "$window_root/logs/game-wine.stdout.txt" 2> "$window_root/logs/game-wine.txt" &
    game_pid=$!
    while kill -0 "$game_pid" 2>/dev/null; do
        if ! kill -0 "$controls_pid" 2>/dev/null; then
            if wait "$controls_pid"; then
                controls_pid=
                printf '%s\n' 'Select + Start requested exit.'
                exit 0
            else
                controls_pid=
                printf '%s\n' 'Controller helper unavailable; see controls.txt. USB mouse/keyboard remain usable.'
                break
            fi
        fi
        sleep 1
    done
    if wait "$game_pid"; then game_status=0; else game_status=$?; fi
    game_pid=
    printf 'Game process exited: %s\n' "$game_status"
    exit "$game_status"
fi
# The probe's result is generated only in this run's dedicated log directory.
cd "$window_root/logs"
rm -f "$window_result"
window_start=$(date +%s)
if timeout -s TERM -k 5s 300s env LD_PRELOAD="$window_app_preload" LD_LIBRARY_PATH="$window_app_libpath" \
    "$window_root/box64" "$wine_dir/bin/wine" \
    "$window_root/$window_exe" > "$window_log-wine.stdout.txt" 2> "$window_log-wine.txt"; then
    window_status=0
else
    window_status=$?
fi
cat "$window_log-wine.txt" "$window_log-wine.stdout.txt"
printf 'Windows probe exit status: %s; elapsed seconds: %s\n' "$window_status" "$(($(date +%s) - window_start))"
if [ -f "$window_result" ]; then cat "$window_result"; fi
if [ "$window_status" = 0 ] && [ -f "$window_result" ] && tr -d '\r' < "$window_result" | grep -qx "$window_marker"; then
    printf 'Windows %s graphics probe: PASS\n' "$window_mode"
else
    printf 'Windows %s graphics probe: FAIL\n' "$window_mode"; exit 1
fi
printf '%s\n' 'GDI runs headlessly. D3D8 modes may briefly show a colored surface via the SDL graphics bridge. No game is launched.'
