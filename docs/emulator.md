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

## School scene and remaining failures

The original game reached its main menu, accepted Play / Instant Action /
Farmland, loaded the level and rendered the 3D school grounds and HUD, including
buildings, trees, road, money and school statistics. Actual X11 frames were
captured privately. Menu pointer movement and some menu interactions worked;
building placement and save/load were not verified. The simulator ran with
software Mesa and Box64 interpreter mode, so these results do not predict
handheld frame rate.

Further CPU tests reproduced marker output followed by status 137 in both
32-bit and 64-bit Windows command programs. None of these experiments produced
a verified recompiler fix: disabling disk cache, interpreting signal handlers,
stronger flag/barrier settings, unmodified Box64 0.3.8, or an experimental
longer Wine server shutdown grace period. The experimental server is retained
only in the private analysis workspace and is not part of the launcher.

A separate GLES bridge attempt used the distributed GL4ES/Crusty/early-EGL
helper with software EGL/GLES and SDL's X11 backend. The isolated test exited 1
with CreateWindow failure (0x80070578), before a successful Direct3D test. This
is not a Mali-driver pass. Earlier CPU and bridge attempts shared a prefix;
those comparisons were invalid because desktop/service state could interfere.
Use a separate prefix for each graphics environment and stop only its owned
services between tests.

Native Mali rendering, a usable recompiler mode, reliable gameplay controls,
sound, saves and actual device performance remain unresolved. The handheld
launcher has not been changed on the strength of these failed experiments.

## Profile reboot validation

The checked-in launcher and rebuilt initramfs were booted with the final
profile. The guest reported four Cortex-A53 CPUs, Linux 4.9.170, glibc 2.40,
4096-byte pages, MemTotal **996496 KiB** (12 KiB above the device report), and
SwapTotal **498240 KiB**, matching the reported swap. Box64 measured the
**24.0 MHz** counter and detected **39-bit** addresses. Wine 10.0 `--version`
again exited zero. This validates the profile reboot, not a second gameplay run.
The VM starts with more available memory than the handheld's full frontend.

The retained private startup check is mirrored in
`tools/emulator/guest-check.sh`. It recreates only this VM's swap file and
checks runtime versions; never execute it on a handheld or another host.
Copy it into the read-only analysis share's `emulator/shared/guest-test.sh`
before preparing the initramfs. The default boot ends at a guest shell.

Captured frames and full traces remain private under the analysis workspace's
`emulator/output` directory. No original game or firmware assets are tracked.

## GL4ES bridge correction (2026-10-13)

Two launcher/bridge defects were reproduced:

- Box64 defaults its desktop libGL override to `SDL_VIDEO_GL_DRIVER`. The
  previous launcher set that to native GLES, so Wine bypassed GL4ES. It now
  sets `BOX64_LIBGL` explicitly to the selected desktop translation library.
- Bundled Crusty's `glXGetFBConfigs` allocates its opaque configuration inside
  the returned pointer array. Wine correctly frees that array and retains the
  configuration, producing a dangling handle. Later `GLX_DRAWABLE_TYPE` queries
  lose the window bit and Wine rejects the pixel format. The new native
  `st-glx-config.c` returns a separate caller-owned pointer array and keeps the
  original backing allocation alive for the process lifetime. It resolves the
  bridge through its explicit library handle: Box64's `RTLD_NEXT` resolution
  otherwise caused recursion in the initial experiment.

`tools/emulator/glx-config-probe.c` reproduced the lifetime bug using the real
ARM64 bridge: DRAWABLE-BEFORE=1 / AFTER=0, exit 4. With the helper it retained
BEFORE=1 / AFTER=1, exit 0. The host fixture also exercises both enumeration
functions after callers free their arrays; its uncorrected control fails.

For the VM's substituted SDL X11 backend, set `SDL_VIDEO_X11_FORCE_EGL=1`.
Without it SDL recursively used the desktop GLX path and failed context
creation. This is a VM setting; the handheld uses SDL's Mali backend.
`tools/emulator/graphics-env.sh` records the working VM library selection.

With the corrected native helper and interpreter/dynarec command experiments,
the actual PE32 Direct3D 8 probe wrote ST-D3D8-PASS: CreateDevice, Clear,
LockRect pixel readback and Present succeeded through GL4ES/GLES. Those sessions
still exited 137 during shutdown, including an interpreter client connected to
a recompiler server. Rendering success is distinct from clean process exit.
A fresh session with all Wine processes in interpreter mode then wrote the
marker again and exited 0. This is the clean GL4ES/GLES rendering baseline;
there was no kernel OOM report in the VM log.
The original game with this GLES bridge has not yet reached a verified scene;
the earlier original-game scene used desktop software Mesa.
