#!/bin/sh
# Install a checksum-pinned portable Wine build beside the CPU diagnostic.
# Does not change firmware, system packages, or game files.
set -eu
runtime_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
wine_name=wine-11.19-amd64-wow64
wine_hash=a1fe858b45d12c9b16b91c7601d9b934f68430f7b6b95e240212ab6e2a198d8a
wine_url="https://github.com/Kron4ek/Wine-Builds/releases/download/11.19/$wine_name.tar.xz"
if [ "$(uname -m)" != aarch64 ]; then
    printf '%s\n' 'Run this installer on the ARM64 handheld.' >&2
    exit 1
fi
for runtime_cmd in curl sha256sum tar df awk mktemp; do
    command -v "$runtime_cmd" >/dev/null || { printf 'Missing command: %s\n' "$runtime_cmd" >&2; exit 1; }
done
mkdir -p "$runtime_root/runtime"
if [ -e "$runtime_root/runtime/$wine_name" ]; then
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
printf '%s\n' 'Downloading portable Wine (approximately 95 MiB)...'
curl --fail --location --proto '=https' --tlsv1.2 --output "$wine_stage/$wine_name.tar.xz" "$wine_url"
printf '%s  %s\n' "$wine_hash" "$wine_stage/$wine_name.tar.xz" | sha256sum -c -
tar -xJf "$wine_stage/$wine_name.tar.xz" --no-same-owner -C "$wine_stage"
test -x "$wine_stage/$wine_name/bin/wine"
test -f "$wine_stage/$wine_name/lib/wine/i386-windows/cmd.exe"
test -f "$wine_stage/$wine_name/lib/wine/x86_64-windows/cmd.exe"
mv "$wine_stage/$wine_name" "$runtime_root/runtime/$wine_name"
printf '%s\n' 'Wine extracted and checksum verified. Run test-wine.sh next.'
