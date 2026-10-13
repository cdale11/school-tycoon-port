#!/bin/sh
# Install a checksum-pinned portable Wine build beside the CPU diagnostic.
# Does not change firmware, system packages, or game files.
set -eu
runtime_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
wine_name=wine-10.0-amd64-wow64
wine_hash=aeebbbf239e548f0136f1cd72a2e109dd9a572a8f703da3c09fa40d74d5c255f
wine_url="https://github.com/Kron4ek/Wine-Builds/releases/download/10.0/$wine_name.tar.xz"
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this installer on the ARM64 handheld.' >&2
    exit 1
fi
for runtime_cmd in curl sha256sum tar df awk mktemp; do
    command -v "$runtime_cmd" >/dev/null || { printf 'Missing command: %s\n' "$runtime_cmd" >&2; exit 1; }
done
mkdir -p "$runtime_root/runtime"
# A dangling link passes neither -e nor -d, but still blocks mv/ln.
# Remove only the link itself; never remove its target or a real user directory.
if [ -L "$runtime_root/runtime/$wine_name" ] && [ ! -e "$runtime_root/runtime/$wine_name" ]; then
    printf 'Removing missing-target Wine link: %s\n' "$(readlink "$runtime_root/runtime/$wine_name")"
    rm -- "$runtime_root/runtime/$wine_name"
fi
if [ -e "$runtime_root/runtime/$wine_name" ] && [ ! -d "$runtime_root/runtime/$wine_name" ]; then
    printf 'Wine installation path is not a directory: %s\n' "$runtime_root/runtime/$wine_name" >&2
    printf '%s\n' 'Leaving the existing file untouched. Move it aside before launching again.' >&2
    exit 1
fi
if [ -d "$runtime_root/runtime/$wine_name" ]; then
    if [ ! -x "$runtime_root/runtime/$wine_name/bin/wine" ] || [ ! -f "$runtime_root/runtime/$wine_name/lib/wine/i386-windows/cmd.exe" ] || [ ! -f "$runtime_root/runtime/$wine_name/lib/wine/x86_64-windows/cmd.exe" ]; then
        printf '%s\n' 'Existing Wine directory is incomplete; leaving it untouched.' >&2
        exit 1
    fi
    printf '%s\n' 'Wine directory already exists; leaving it untouched.'
    exit 0
fi
available_kb=$(df -Pk "$runtime_root" | awk 'END {print $4}')
if [ "$available_kb" -lt 2097152 ]; then
    printf '%s\n' 'Allow at least 2 GiB free for Wine and its test prefix.' >&2
    exit 1
fi
wine_stage=$(mktemp -d "$runtime_root/runtime/.wine-stage.XXXXXX")
trap 'rm -rf "$wine_stage"' EXIT HUP INT TERM
mkdir -p "$runtime_root/runtime/downloads"
wine_archive="$runtime_root/runtime/downloads/$wine_name.tar.xz"
if [ -f "$wine_archive" ]; then
    if printf '%s  %s\n' "$wine_hash" "$wine_archive" | sha256sum -c -; then
        printf '%s\n' 'Reusing the cached, checksum-verified Wine download.'
    else
        printf '%s\n' 'Cached Wine archive failed verification; downloading a fresh copy.'
        rm -- "$wine_archive"
    fi
fi
if [ ! -f "$wine_archive" ]; then
    printf '%s\n' 'Downloading portable Wine 10.0...'
    curl --fail --location --proto '=https' --tlsv1.2 --output "$wine_stage/$wine_name.tar.xz" "$wine_url"
    printf '%s  %s\n' "$wine_hash" "$wine_stage/$wine_name.tar.xz" | sha256sum -c -
    mv "$wine_stage/$wine_name.tar.xz" "$wine_archive"
fi
printf '%s\n' 'Extracting verified Wine archive (approximately 645 MiB)...'
if ! tar -xJf "$wine_archive" --no-same-owner -C "$wine_stage"; then
    printf '%s\n' 'Wine extraction failed. The verified download is retained for the next attempt.' >&2
    exit 1
fi
test -x "$wine_stage/$wine_name/bin/wine"
test -f "$wine_stage/$wine_name/lib/wine/i386-windows/cmd.exe"
test -f "$wine_stage/$wine_name/lib/wine/x86_64-windows/cmd.exe"
mv "$wine_stage/$wine_name" "$runtime_root/runtime/$wine_name"
printf '%s\n' 'Wine installation completed and checksum verified.'
