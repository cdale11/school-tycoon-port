# RG34XXSP cloud test environment

The emulator uses the user's measured device specifications in
`tools/emulator/rg34xxsp.json`: four Cortex-A53 cores, Linux 4.9.170,
4 KiB pages, 39-bit virtual addresses, glibc 2.40 and about 1 GiB RAM.
Guest memory is adjusted for the different kernel's reserved memory; measured
MemTotal and swap must be recorded for each run.

This is QEMU's `virt` board, **not an emulated H700 or Mali-G31**.
The publicly available RG34XXSP Gladiator II 20250813 filesystem uses
Buildroot 2024.11 and glibc 2.40, like the reported Scarab build, but is an
older firmware release. Do not call it an exact Scarab image. Software-rendered
graphics tests cannot certify the handheld's native Mali driver or performance.

## Prepared private inputs

Keep firmware images, original game assets, VM disks and runtime installations
outside this repository. The current cloud workspace retains them under
`/workspace/school-tycoon-analysis/emulator`.

- QEMU 10.0.13 from signature-verified Debian trixie packages, extracted locally.
- Linux tag `v4.9.170`, commit `ad387ec4bcdf087faaf084a3db8f681295463b01`.
  Build ARM64 defconfig with `CONFIG_ARM64_VA_BITS_39=y`,
  `CONFIG_ARM64_VA_BITS=39`, `CONFIG_HZ_100=y`, built-in virtio block/MMIO,
  9p/virtio and gzip squashfs. GCC 14 build uses `HOSTCFLAGS=-fcommon`
  for the older bundled device-tree compiler and `KCFLAGS=-Wno-error`.
- Official KNULLI image downloaded from the `20250813` GitHub release;
  publisher SHA256 is recorded in the profile. Extract `boot/batocera` from
  the GPT `boot-resource` FAT partition (starting at sector 149504).
  This squashfs remains a read-only guest root disk.
- ARM64 static BusyBox in a gzip/newc initramfs with `tools/emulator/init`.
- Separate ext4 guest data disk. When creating it with current e2fsprogs,
  disable `metadata_csum_seed` and `orphan_file` for Linux 4.9 compatibility.
- Read-only host analysis share. Guest startup reads the private test script at
  `/userdata/host/emulator/shared/guest-test.sh`.

Run with `LD_LIBRARY_PATH` pointing to the extracted QEMU native libraries:

```sh
python3 tools/emulator/launch.py \
  --vm-dir /workspace/school-tycoon-analysis/emulator \
  --analysis-dir /workspace/school-tycoon-analysis \
  --qemu /workspace/school-tycoon-analysis/emulator/qemu/usr/bin/qemu-system-aarch64
```

The VM has no network device. Copy executable runtimes and game test data into
its ext4 disk before running them. An initial Box64 execution directly from
9p crashed before startup; the same binary copied to ext4 successfully printed
its version and ran Wine 10.0 `--version`. This was a VM storage issue.

## Evidence so far

The first VM boot reported Linux 4.9.170, four Cortex-A53 CPUs, 4096-byte pages,
and glibc 2.40. Box64 detected 39-bit addresses and Wine reported `wine-10.0`.
Fresh prefix creation and original-game execution are still under investigation.
No emulator gameplay or handheld gameplay pass is claimed.
