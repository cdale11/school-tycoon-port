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

## First original-game run (2026-10-13)

With executable runtimes copied to guest ext4 and native ARM64 Xvfb/Mesa
libraries installed, the following checks ran inside the Linux 4.9.170 VM:

- 32-bit Windows `cmd.exe`: exact `ST-PREFIX32-PASS` output and exit 0
  under Box64 interpreter mode.
- Native GLX: Mesa 25.0.7 llvmpipe / LLVM 19.1.7, OpenGL 4.5 and GLES 3.2.
- Original-game-era Direct3D 8 probe: `ST-D3D8-PASS`, exit 0, including
  clear, pixel readback and present.
- Original SchoolTycoon.exe plus the complete private asset set: created its
  session manager, initialized WineD3D and displayed the animated school intro.
  Main-menu interaction and gameplay are not yet verified.

These are actual ARM64 guest executions, not substituted Wine commands or host
x86 Wine tests. The initial VM used 1024 MiB RAM and a 62.5 MHz virtual counter;
its MemTotal was 1013908 KiB. The profile adjusts the next boot to 1007 MiB and
sets the reported 24 MHz counter. These adjustments still require a measured
reboot; do not imply the first run matched those values exactly.

Dynamic-recompiler command tests printed their markers but exited 137.
A syscall trace showed Wine's server polling process existence and issuing
SIGKILL during process shutdown. This demonstrates a server-initiated kill in
this VM, not a confirmed handheld OOM diagnosis. Wine 10.0's
`server/process.c:process_sigkill` applies a short shutdown grace period.
`BOX64_DYNAREC_INTERP_SIGNAL=1` did not fix that test. Disk-cache/shutdown
behavior is being investigated; no new recompiler default is established.

The X server must start with the correct native Mesa library and driver paths.
An earlier server created before those dependencies were installed had no GLX
visuals. Changing DISPLAY within an already-running Wine session also produced
an invalid comparison; restart that prefix's services when changing displays.

`tools/emulator/capture.c` captures the X11 root window as a private PPM frame.
To extract a consistent file from an active guest disk, temporarily freeze
**only that VM filesystem**, read it with host debugfs, then immediately thaw it.
BusyBox uses `fsfreeze --freeze /userdata` and `--unfreeze /userdata`.
Never run a writable filesystem repair tool against a live VM disk.
