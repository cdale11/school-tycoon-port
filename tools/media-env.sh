# Source in the owned launcher/prefix setup after native media installation.
media_base_overrides='winewayland.drv=;iexplore.exe=;winemenubuilder.exe,mscoree,mshtml='
if [ -f "$media_root/runtime/directmusic-ready" ]; then
    export WINEDLLOVERRIDES="dmband,dmcompos,dmime,dmloader,dmscript,dmstyle,dmsynth,dmusic,dswave=n;$media_base_overrides"
else
    export WINEDLLOVERRIDES="$media_base_overrides"
fi
