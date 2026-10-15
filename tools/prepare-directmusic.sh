#!/bin/sh
# Install checksum-pinned legacy DirectMusic into this port's PE32 prefix only.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
dest="$root/prefix-wine-10.0/drive_c/windows/syswow64"
sdk_hash=f6d191e89a963d7cca34f169d30f49eab99c1ed3bb92da73ec43617caaa1e93f
sdk_name=directx_feb2010_redist.exe
manifest() {
cat <<'HASHES'
9b47be4207fdf4938ed2efe147267368156eff3ef858532894dc65994431ab78  dmband.dll
710f66714db0e5c6188d9a0f39e21f06ddcc9ded23e5457e07f8c04ed09de697  dmcompos.dll
69c92f231a9198dfd036c3904bdd3a1900c7f1ed0ce416b9dd3cd65c20b32fbf  dmime.dll
544800d356d887b9dd560807b93e8031c9dffd8fe2c70c515ee33105d9867fee  dmloader.dll
28d548356bf614035ad26ec572533c09bc88d9cd693a360a2cc74a4b8ea3172a  dmscript.dll
c7ef00e4f266b88d65a93ee043d990ede09668a2c356896f5d488831dd62505e  dmstyle.dll
eb164cc9722e4a04d11bd77b7b98c57a7b76fc5564abbf35c5ee693c1b7571fa  dmsynth.dll
33b4ffc6c91536fb5e8d3699ba50086b7dcc3853436c8ba95d235f0e380aae07  dmusic.dll
73174f7c2d273f0a371fa76abd3057be4d103aea4ccb087999d972ddadb4d82a  dswave.dll
HASHES
}
check() { [ -d "$dest" ] && (cd "$dest"; manifest | sha256sum -c -) >/dev/null 2>&1; }
if [ "${1:-}" = --check ]; then check; exit $?; fi
mkdir -p "$root/runtime/downloads"
if check; then printf '%s\n' "$sdk_hash" > "$root/runtime/directmusic-ready"; exit 0; fi
cab_root="$root/runtime/cabextract"
test -x "$cab_root/cabextract"
test -f "$cab_root/libmspack.so.0"
archive="$root/runtime/downloads/$sdk_name"
if [ -L "$archive" ] || { [ -e "$archive" ] && [ ! -f "$archive" ]; }; then
    printf '%s\n' 'DirectMusic cache path is not a regular file; leaving it untouched.' >&2; exit 1
fi
stage=$(mktemp -d "$root/runtime/.directmusic-stage.XXXXXX")
trap 'rm -rf "$stage"' EXIT
trap 'exit 143' HUP TERM
trap 'exit 130' INT
if [ -f "$archive" ] && ! printf '%s  %s\n' "$sdk_hash" "$archive" | sha256sum -c -; then
    printf '%s\n' 'DirectMusic cache failed verification; replacing only this cached download.'
    rm -- "$archive"
fi
if [ ! -f "$archive" ]; then
    free_kb=$(df -Pk "$root" | awk 'END {print $4}')
    [ "$free_kb" -ge 163840 ] || { printf '%s\n' 'Allow 160 MiB free for DirectMusic setup.' >&2; exit 1; }
    printf '%s\n' 'Downloading checksum-pinned Microsoft DirectX February 2010 runtime...'
    # Official URL is retired; this Winetricks mirror serves the identical pinned redistributable.
    curl --fail --location --connect-timeout 30 --max-time 900 --retry 2 --proto '=https' --tlsv1.2 --output "$stage/$sdk_name" \
        "https://files.holarse-linuxgaming.de/mirrors/microsoft/$sdk_name"
    printf '%s  %s\n' "$sdk_hash" "$stage/$sdk_name" | sha256sum -c -
    mv "$stage/$sdk_name" "$archive"
fi
cab() { LD_PRELOAD= LD_AUDIT= LD_LIBRARY_PATH="$cab_root" "$cab_root/cabextract" "$@"; }
cab -q -L -F dxnt.cab -d "$stage" "$archive"
manifest | while read -r digest name; do
    cab -q -L -F "$name" -d "$stage" "$stage/dxnt.cab"
done
(cd "$stage"; manifest | sha256sum -c -)
mkdir -p "$dest" "$root/runtime/directmusic-backups"
# Preflight all destinations: mv must never interpret a DLL path as a directory.
manifest | while read -r digest name; do
    target="$dest/$name"
    if [ -d "$target" ] || { [ -e "$target" ] && [ ! -f "$target" ] && [ ! -L "$target" ]; }; then
        printf 'Unexpected DLL path type: %s\n' "$target" >&2; exit 1
    fi
done
manifest | while read -r digest name; do
    target="$dest/$name"
    if [ -f "$target" ]; then
        old_hash=$(sha256sum "$target" | awk '{print $1}')
        backup="$root/runtime/directmusic-backups/$name.$old_hash"
        if [ ! -e "$backup" ]; then cp -p "$target" "$backup"; fi
    elif [ -e "$target" ] && [ ! -L "$target" ]; then
        printf 'Unexpected DLL path type: %s\n' "$target" >&2; exit 1
    fi
    cp "$stage/$name" "$stage/publish-$name"
    # Rename replaces a prefix symlink, never its original Wine-runtime target.
    mv -f "$stage/publish-$name" "$target"
done
check
printf '%s\n' "$sdk_hash" > "$root/runtime/directmusic-ready"
rm -f "$root/runtime/prefix-media-verified"
printf '%s\n' 'Verified native DirectMusic installed; Wine DirectSound and firmware remain unchanged.'
