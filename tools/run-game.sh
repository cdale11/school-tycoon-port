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
# Preserve an existing per-user audio server before replacing its runtime dir
# for private Wayland. libpulse otherwise searches the new (empty) directory.
if [ -z "${PULSE_SERVER:-}" ] && [ -n "${XDG_RUNTIME_DIR:-}" ] && [ -S "$XDG_RUNTIME_DIR/pulse/native" ]; then
    export PULSE_SERVER="unix:$XDG_RUNTIME_DIR/pulse/native"
    printf '%s\n' 'Preserved native PulseAudio connection for private display session.'
fi
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
weston_birth=
controls_pid=
controls_birth=
desktop_pid=
desktop_birth=
telemetry_pid=
telemetry_birth=
stdout_pid=
stdout_birth=
stderr_pid=
stderr_birth=
pipe_dir=
game_pid=
game_birth=
uptime_seconds() {
    # Startup/shutdown deadlines must survive NTP or manual wall-clock changes.
    awk '{print int($1)}' /proc/uptime
}
child_start() {
    awk '{sub(/^.*\) /, ""); print $20}' "/proc/$1/stat" 2>/dev/null || :
}
child_alive() {
    [ -n "$1" ] && [ -n "$2" ] && [ "$(child_start "$1")" = "$2" ]
}
# Stop only our captured children; do not block indefinitely on stuck IO.
stop_child() {
    child_pid=$1; child_birth=$2
    child_alive "$child_pid" "$child_birth" || return 0
    kill "$child_pid" 2>/dev/null || :
    child_stop_start=$(uptime_seconds)
    while child_alive "$child_pid" "$child_birth"; do
        if [ "$(($(uptime_seconds)-child_stop_start))" -ge 2 ]; then
            kill -KILL "$child_pid" 2>/dev/null || :
            sleep 0.1
            break
        fi
        sleep 0.05
    done
    if child_alive "$child_pid" "$child_birth"; then
        printf 'Child %s remains blocked after SIGKILL; continuing cleanup.\n' "$child_pid"
    else
        wait "$child_pid" 2>/dev/null || :
    fi
}
cleanup_window() {
    cleanup_status=$?
    trap - EXIT HUP INT TERM
    printf 'Session finished: exit=%s phase=%s time=%s\n' "$cleanup_status" "$session_phase" "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    if [ "$cleanup_status" -ge 128 ]; then
        dmesg 2>/dev/null | tail -n 80 > "$window_root/logs/kernel.txt" || :
    fi
    stop_child "$telemetry_pid" "$telemetry_birth"
    stop_child "$desktop_pid" "$desktop_birth"
    stop_child "$controls_pid" "$controls_birth"
    stop_child "$game_pid" "$game_birth"
    BOX64_LOG=0 timeout -s TERM -k 5s 15s "$window_root/box64" "$wine_dir/bin/wineserver" -k || :
    stop_child "$stdout_pid" "$stdout_birth"
    stop_child "$stderr_pid" "$stderr_birth"
    stop_child "$weston_pid" "$weston_birth"
    if [ -n "$pipe_dir" ]; then rm -f "$pipe_dir/stdout" "$pipe_dir/stderr"; rmdir "$pipe_dir" || :; fi
}

export WINEPREFIX="$window_root/prefix-wine-10.0"
export WINEARCH=win64 BOX64_DYNAREC=0 BOX64_LOG=0
# Bound glibc heap arenas on the 1 GiB handheld; rendering stays single-context.
export MALLOC_ARENA_MAX=2
export BOX64_PATH="$window_root:$wine_dir/bin:/bin:/usr/bin"
export BOX64_LD_LIBRARY_PATH="$window_root/runtime/x86_64:$wine_dir/lib/wine/x86_64-unix"
media_root="$window_root"
. "$window_root/media-env.sh"
printf 'Media stack: %s\n' "$media_stack"
export WINEDEBUG='-all,err+all'
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
weston_pid=$!; weston_birth=$(child_start "$weston_pid")
window_wait_start=$(uptime_seconds)
while [ ! -S /tmp/.X11-unix/X0 ]; do
    if ! child_alive "$weston_pid" "$weston_birth" || [ "$(($(uptime_seconds) - window_wait_start))" -ge 30 ]; then
        printf 'Weston/Xwayland did not become ready. See %s-weston.txt.\n' "$window_log"
        cat "$window_root/logs/$window_log-weston.txt"
        exit 1
    fi
    sleep 0.2
done
export DISPLAY=:0 WAYLAND_DISPLAY=school-tycoon-window
# Repair Wine/RpcSs in the verified private X11 session, before Crusty.
session_phase=prefix-readiness
printf '%s\n' 'Phase: prefix-readiness'
sh "$window_root/prepare-prefix.sh"
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
desktop_pid=$!; desktop_birth=$(child_start "$desktop_pid")
desktop_start=$(uptime_seconds)
while ! grep -q '^ST-DESKTOP-READY' "$window_root/logs/desktop-ready.txt" 2>/dev/null; do
    if ! child_alive "$desktop_pid" "$desktop_birth" || [ "$(($(uptime_seconds) - desktop_start))" -ge 180 ]; then
        if ! child_alive "$desktop_pid" "$desktop_birth"; then
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
session_phase=graphics-configuration
window_gl=gl4es_glxpass
window_app_libpath="$weston_dir/lib_aarch64/graphics/$window_gl:$weston_dir/lib_aarch64/graphics/crusty_glx:$weston_dir/lib_aarch64"
test -f "$window_root/runtime/graphics/libst-egl-prime.so"
test -f "$window_root/runtime/graphics/libst-glx-config.so"
export ST_CRUSTY_LIBRARY="$weston_dir/lib_aarch64/graphics/crusty_glx/libcrusty.so"
window_app_preload="$window_root/runtime/graphics/libst-glx-config.so:$window_root/runtime/graphics/libst-egl-prime.so:$ST_CRUSTY_LIBRARY"
    test -f "$window_root/runtime/graphics/libst-gl-present.so"
    window_app_preload="$window_root/runtime/graphics/libst-gl-present.so:$window_app_preload"
    window_gles=30
    # Crusty supplies one native context rather than GLX context switching.
    export WINE_D3D_CONFIG=csmt=0 LIBGL_NOTEXARRAY=1
export ST_EGL_TARGET=SchoolTycoon.exe
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
# Print generated shader sources on failure; the Mali linker can return an
# empty info log. This is failure-only output, bounded by the session logger.
export LIBGL_LOGSHADERERROR=1
export WINEDEBUG='-all,err+all'
if [ "${ST_DEBUG:-0}" = 1 ]; then
    export WINEDEBUG='-all,err+all,warn+d3d8,warn+wined3d,warn+dsound,warn+dmusic,warn+dmloader,fixme+dmscript'
fi
# Full game runs without a diagnostic timeout, from its asset directory.
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
controls_pid=$!; controls_birth=$(child_start "$controls_pid")
test -x "$window_root/st-log"
pipe_dir=$(mktemp -d "$window_root/logs/.pipes.XXXXXX")
mkfifo "$pipe_dir/stdout" "$pipe_dir/stderr"
"$window_root/st-log" "$window_root/logs/game-wine.stdout.txt" "$window_root/logs/game-wine.stdout.startup.txt" < "$pipe_dir/stdout" & stdout_pid=$!; stdout_birth=$(child_start "$stdout_pid")
"$window_root/st-log" "$window_root/logs/game-wine.txt" "$window_root/logs/game-wine.startup.txt" < "$pipe_dir/stderr" & stderr_pid=$!; stderr_birth=$(child_start "$stderr_pid")
cd "$window_root/gamedata"
env LD_PRELOAD="$window_app_preload" LD_LIBRARY_PATH="$window_app_libpath" \
    "$window_root/box64" "$wine_dir/bin/wine" ./SchoolTycoon.exe \
    > "$pipe_dir/stdout" 2> "$pipe_dir/stderr" &
game_pid=$!; game_birth=$(child_start "$game_pid")
printf 'Game PID: %s; heap arenas: %s; csmt=0; debug=%s\n' "$game_pid" "$MALLOC_ARENA_MAX" "${ST_DEBUG:-0}"
sh "$window_root/sample-session.sh" "$game_pid" "$window_root/logs/memory.csv" & telemetry_pid=$!; telemetry_birth=$(child_start "$telemetry_pid")
while child_alive "$game_pid" "$game_birth"; do
    if ! child_alive "$weston_pid" "$weston_birth"; then
        session_phase=display-lost
        printf '%s\n' 'Private display server stopped; ending the game session.'
        exit 1
    fi
    # Wine capability probes can reject an individual format. Repeated native
    # allocation AND framebuffer failures are different: this run cannot render.
    render_oom=$(grep -c 'GL_OUT_OF_MEMORY' "$window_root/logs/game-wine.txt" 2>/dev/null || :)
    render_fbo=$(grep -c 'GL_INVALID_FRAMEBUFFER_OPERATION' "$window_root/logs/game-wine.txt" 2>/dev/null || :)
    if [ "${render_oom:-0}" -ge 16 ] && [ "${render_fbo:-0}" -ge 16 ]; then
        session_phase=rendering-failed
        printf '%s\n' 'Repeated GPU allocation/framebuffer failures; ending this run. Startup logs are preserved.'
        dmesg 2>/dev/null | tail -n 80 > "$window_root/logs/kernel.txt" || :
        exit 1
    fi
    # Crusty can continue after SDL window/context failure and leave Wine alive
    # indefinitely. Return to the frontend so its normal game-end hooks run.
    if grep -Eq 'ST-GRAPHICS-FAILED|Failed to get a GL context for adapter' "$window_root/logs/game-wine.txt" 2>/dev/null ||
       grep -Eq 'Could not create SDL Window:|Could not create OpenGL Context:' "$window_root/logs/game-wine.stdout.txt" 2>/dev/null; then
        session_phase=graphics-failed
        printf '%s\n' 'Native graphics initialization failed; stopping this session. See game-wine logs.'
        awk '/^(MemAvailable|CmaTotal|CmaFree):/' /proc/meminfo
        dmesg 2>/dev/null | tail -n 80 > "$window_root/logs/kernel.txt" || :
        exit 1
    fi
    if [ -n "$desktop_pid" ] && ! child_alive "$desktop_pid" "$desktop_birth"; then
        if wait "$desktop_pid"; then
            printf '%s\n' 'Desktop readiness helper retired after game window creation.'
        else
            printf '%s\n' 'Desktop readiness helper ended unexpectedly; see desktop.txt.'
        fi
        desktop_pid=
    fi
    if ! child_alive "$controls_pid" "$controls_birth"; then
        if wait "$controls_pid"; then
            controls_pid=
            printf '%s\n' 'Controller exit shortcut requested exit.'
            exit 0
        else
            controls_pid=
            printf '%s\n' 'Controller helper unavailable; see controls.txt. Handheld USB input is unverified.'
            exit 1
        fi
    fi
    sleep 1
done
if wait "$game_pid"; then game_status=0; else game_status=$?; fi
game_pid=
printf 'Game process exited: %s\n' "$game_status"
exit "$game_status"
