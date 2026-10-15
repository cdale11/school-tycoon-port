# Verified CAB runtime build input

`cab-runtime.tar.gz` SHA256:
`5cf7f9a7f452df470c6e7f9aa917b416a21d4cee37c8a57b18d567c28d26e510`.

Unchanged Debian bookworm ARM64 cabextract 1.9-3 and libmspack 0.11-1,
with copyright/license files and complete corresponding Debian source archives.
Debian InRelease signatures were checked against the Debian archive keyring;
package and source hashes were checked through the signed indexes. See the
bundle's PROVENANCE.txt for URLs and hashes. This free-software build input
contains no Microsoft DLLs or original game assets.

Pass `--cab-runtime build-inputs/cab-runtime.tar.gz` to build-handheld.py.
