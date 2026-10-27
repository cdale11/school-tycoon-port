# Source in the owned launcher/prefix setup. The build selects the media stack.
media_base_overrides='winewayland.drv=;iexplore.exe=;winemenubuilder.exe,mscoree,mshtml='
media_stack=builtin
media_preference=b
if [ -f "$media_root/runtime/directmusic-enabled" ] ||
   { [ -f "$media_root/experimental-directmusic.txt" ] && [ "$(cat "$media_root/experimental-directmusic.txt")" = 1 ]; }; then
    media_stack=native
fi
# Native DirectSound distorted the interpreter fixture. Keep that CPU fallback
# on builtins; an explicit audio fallback also preserves the installed DLLs.
if [ "${ST_CPU_MODE:-dynarec}" = interpreter ] ||
   { [ -f "$media_root/audio-mode.txt" ] && [ "$(cat "$media_root/audio-mode.txt")" = builtin ]; }; then
    media_stack=builtin
fi
if [ "$media_stack" = native ]; then
    test -f "$media_root/prepare-directmusic.sh" || { printf '%s\n' 'Native media installer is missing; extract the complete package.' >&2; exit 1; }
    # A fresh prefix first boots with builtins; switch only after installation.
    if [ -f "$media_root/runtime/directmusic-ready" ]; then media_preference=n; fi
fi
# Explicit builtin preference also ignores DLLs left by an earlier experiment.
export WINEDLLOVERRIDES="dmband,dmcompos,dmime,dmloader,dmscript,dmstyle,dmsynth,dmusic,dsound,dswave=$media_preference;$media_base_overrides"
