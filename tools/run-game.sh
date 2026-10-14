#!/bin/sh
# Original-game session. Weston supplies private input; native SDL/Mali presents graphics.
set -eu
window_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
wine_dir="$window_root/runtime/wine-10.0-amd64-wow64"
weston_dir="$window_root/runtime/weston-0.2"
weston_image=/userdata/system/.local/share/PortMaster/libs/weston_pkg_0.2.squashfs
weston_hash=0bccaeebc11296bb1fe4278c9677cb82965c5a04bcd63745fa09d78228bc1a93
session_phase=preflight
window_mode=game
window_log=game
window_exe=gamedata/SchoolTycoon.exe
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'School Tycoon requires ARM64 KNULLI.' >&2; exit 1
fi
for window_cmd in unsquashfs sha256sum timeout date grep tr mktemp mkfifo awk head tail; do
    command -v "$window_cmd" >/dev/null || { printf 'Missing command: %s\n' "$window_cmd" >&2; exit 1; }
done
mkdir -p "$window_root/logs"
exec > "$window_root/logs/$window_log.txt" 2>&1
printf '%s\n' 'School Tycoon game session'
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
session_phase=runtime
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
desktop_pid=
telemetry_pid=
stdout_pid=
stderr_pid=
pipe_dir=
game_pid=
cleanup_window() {
    cleanup_status=$?
    trap - EXIT HUP INT TERM
    printf 'Session finished: exit=%s phase=%s time=%s\n' "$cleanup_status" "$session_phase" "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    if [ "$cleanup_status" -ge 128 ]; then
        dmesg 2>/dev/null | tail -n 80 > "$window_root/logs/kernel.txt" || :
    fi
    if [ -n "$telemetry_pid" ]; then kill "$telemetry_pid" 2>/dev/null || :; wait "$telemetry_pid" 2>/dev/null || :; fi
    if [ -n "$desktop_pid" ]; then kill "$desktop_pid" 2>/dev/null || :; wait "$desktop_pid" 2>/dev/null || :; fi
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
    for log_pid in "$stdout_pid" "$stderr_pid"; do
        if [ -n "$log_pid" ]; then kill "$log_pid" 2>/dev/null || :; wait "$log_pid" 2>/dev/null || :; fi
    done
    if [ -n "$pipe_dir" ]; then rm -f "$pipe_dir/stdout" "$pipe_dir/stderr"; rmdir "$pipe_dir" || :; fi

}
export WINEPREFIX="$window_root/prefix-wine-10.0"
export WINEARCH=win64 BOX64_DYNAREC=0 BOX64_LOG=0
# Bound glibc heap arenas on the 1 GiB handheld; rendering stays single-context.
export MALLOC_ARENA_MAX=2
export BOX64_PATH="$window_root:$wine_dir/bin:/bin:/usr/bin"
export BOX64_LD_LIBRARY_PATH="$window_root/runtime/x86_64:$wine_dir/lib/wine/x86_64-unix"
export WINEDLLOVERRIDES='winewayland.drv=;iexplore.exe=;winemenubuilder.exe,mscoree,mshtml='
export WINEDEBUG='err+all'
trap cleanup_window EXIT
trap 'exit 130' INT
trap 'exit 143' HUP TERM
session_phase=display-start
printf '%s\n' 'Phase: display-start'
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
    session_phase=prefix-readiness
    printf '%s\n' 'Phase: prefix-readiness'
    sh "$window_root/prepare-prefix.sh"
fi
# Verify the actual private X11 desktop, not just a headless cmd.exe.
# The helper exits after the original game window appears, reclaiming its memory.
session_phase=desktop-readiness
printf '%s\n' 'Phase: desktop-readiness (up to 180 seconds)'
rm -f "$window_root/logs/desktop-ready.txt"
test -f "$window_root/st-desktop-ready.exe"
(
    cd "$window_root/logs"
    exec "$window_root/box64" "$wine_dir/bin/wine" "$window_root/st-desktop-ready.exe"
) > "$window_root/logs/desktop.txt" 2>&1 &
desktop_pid=$!
desktop_start=$(date +%s)
while ! grep -q '^ST-DESKTOP-READY' "$window_root/logs/desktop-ready.txt" 2>/dev/null; do
    if ! kill -0 "$desktop_pid" 2>/dev/null || [ "$(($(date +%s) - desktop_start))" -ge 180 ]; then
        if ! kill -0 "$desktop_pid" 2>/dev/null; then
            if wait "$desktop_pid"; then desktop_status=0; else desktop_status=$?; fi
            desktop_pid=
            printf 'Desktop helper exited before readiness: %s\n' "$desktop_status"
        fi
        printf '%s\n' 'Wine desktop did not become ready; see desktop.txt. Game not launched.'
        exit 1
    fi
    sleep 0.2
done
printf '%s\n' 'Private Wine desktop is ready.'
window_app_libpath="$LD_LIBRARY_PATH"
window_app_preload=
if [ "$window_mode" != gdi ]; then
    window_gl=gl4es_glxpass
    if [ "$window_mode" = core4es ]; then window_gl=core4es_glxpass; fi
    window_app_libpath="$weston_dir/lib_aarch64/graphics/$window_gl:$weston_dir/lib_aarch64/graphics/crusty_glx:$weston_dir/lib_aarch64"
    test -f "$window_root/runtime/graphics/libst-egl-prime.so"
    test -f "$window_root/runtime/graphics/libst-glx-config.so"
    export ST_CRUSTY_LIBRARY="$weston_dir/lib_aarch64/graphics/crusty_glx/libcrusty.so"
    window_app_preload="$window_root/runtime/graphics/libst-glx-config.so:$window_root/runtime/graphics/libst-egl-prime.so:$ST_CRUSTY_LIBRARY"
    window_gles=20
    if [ "$window_gl" = gl4es_glxpass ]; then
        test -f "$window_root/runtime/graphics/libst-gl-present.so"
        window_app_preload="$window_root/runtime/graphics/libst-gl-present.so:$window_app_preload"
        window_gles=30
        # Crusty supplies one native context rather than GLX context switching.
        export WINE_D3D_CONFIG=csmt=0 LIBGL_NOTEXARRAY=1
    fi
    if [ "$window_mode" = game ]; then
        export ST_EGL_TARGET=SchoolTycoon.exe
    else
        export ST_EGL_TARGET=windows-d3d8-probe.exe
    fi
    export CRUSTY_LIBEGL=/usr/lib/libEGL.so.1
    export CRUSTY_LIBSDL=/usr/lib/libSDL2-2.0.so.0
    export CRUSTY_GL4ES=1 CRUSTY_WLMODE=0 CRUSTY_WLDISP=school-tycoon-window
    export SDL_VIDEODRIVER=mali
    # Request ES3 explicitly for native framebuffer copies, while GL4ES uses
    # its ES2 translation backend. Keep native library resolution explicit.
    # These are native ARM libraries, never x86 copies from Wine.
    export CRUSTY_GLES="$window_gles" SDL_OPENGL_ES_DRIVER=1
    export SDL_VIDEO_EGL_DRIVER=/usr/lib/libEGL.so.1
    export SDL_VIDEO_GL_DRIVER=/usr/lib/libGLESv2.so.2
    # Box64 otherwise inherits SDL_VIDEO_GL_DRIVER as its desktop libGL.
    # Wine needs GL4ES's OpenGL/GLX exports; SDL itself needs native GLES.
    export BOX64_LIBGL="$weston_dir/lib_aarch64/graphics/$window_gl/libGL.so.1"
    export LIBGL_EGL=/usr/lib/libEGL.so.1 LIBGL_GLES=/usr/lib/libGLESv2.so.2
    export LIBGL_ES=2 CRUSTY_SHOW_CURSOR=1
    export WINEDEBUG='err+all'
    if [ "${ST_DEBUG:-0}" = 1 ]; then
        export WINEDEBUG='err+all,warn+d3d8,warn+wined3d'
    fi
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
    if [ "${ST_DEBUG:-0}" = 1 ]; then export BOX64_LOG=1; fi
    session_phase=game
    printf 'Launching SchoolTycoon.exe; CPU mode: %s\n' "${ST_CPU_MODE:-interpreter}"
    printf '%s\n' 'Graphics configuration is experimental; this is not a verified playable port.'
    export SDL_GAMECONTROLLERCONFIG_FILE="${SDL_GAMECONTROLLERCONFIG_FILE:-/userdata/system/.local/share/PortMaster/knulli/gamecontrollerdb.txt}"
    LD_LIBRARY_PATH="$weston_dir/lib_aarch64" "$window_root/st-controls" > "$window_root/logs/controls.txt" 2>&1 &
    controls_pid=$!
    test -x "$window_root/st-log"
    pipe_dir=$(mktemp -d "$window_root/logs/.pipes.XXXXXX")
    mkfifo "$pipe_dir/stdout" "$pipe_dir/stderr"
    "$window_root/st-log" "$window_root/logs/game-wine.stdout.txt" < "$pipe_dir/stdout" & stdout_pid=$!
    "$window_root/st-log" "$window_root/logs/game-wine.txt" < "$pipe_dir/stderr" & stderr_pid=$!
    cd "$window_root/gamedata"
    env LD_PRELOAD="$window_app_preload" LD_LIBRARY_PATH="$window_app_libpath" \
        "$window_root/box64" "$wine_dir/bin/wine" ./SchoolTycoon.exe \
        > "$pipe_dir/stdout" 2> "$pipe_dir/stderr" &
    game_pid=$!
    printf 'Game PID: %s; heap arenas: %s; csmt=0; debug=%s\n' "$game_pid" "$MALLOC_ARENA_MAX" "${ST_DEBUG:-0}"
    sh "$window_root/sample-session.sh" "$game_pid" "$window_root/logs/memory.csv" & telemetry_pid=$!
    while kill -0 "$game_pid" 2>/dev/null; do
        if [ -n "$desktop_pid" ] && ! kill -0 "$desktop_pid" 2>/dev/null; then
            if wait "$desktop_pid"; then
                printf '%s\n' 'Desktop readiness helper retired after game window creation.'
            else
                printf '%s\n' 'Desktop readiness helper ended unexpectedly; see desktop.txt.'
            fi
            desktop_pid=
        fi
        if ! kill -0 "$controls_pid" 2>/dev/null; then
            if wait "$controls_pid"; then
                controls_pid=
                printf '%s\n' 'Controller exit shortcut requested exit.'
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
