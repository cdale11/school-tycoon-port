# School Tycoon for RG34XXSP

Work toward running the original PC School Tycoon on an Anbernic RG34XXSP
with KNULLI Scarab through PortMaster.

**Status: feasibility and CPU runtime testing. There is no playable port yet.**
Windows compatibility, Direct3D 8 graphics, audio, controller input, and handheld
performance still need validation.

## Test the ARM64 CPU runtime

Download [school-tycoon-runtime-probe.tar.gz](releases/school-tycoon-runtime-probe.tar.gz)
and extract it into `/userdata/roms/ports/` on the handheld. Through SSH, run:

```sh
sh /userdata/roms/ports/school-tycoon-runtime-probe/test-box64.sh
```

Inspect or share the resulting log:

```text
/userdata/roms/ports/school-tycoon-runtime-probe/logs/box64.txt
```

Success ends with `CPU diagnostic completed successfully.` If it fails, retain
the log and terminal error. The script writes only its local log directory; it
does not install packages or change firmware. This diagnostic checks x86_64
translation, not Windows game compatibility.

The package includes a generic ARM64 Box64 build, its MIT license, and a small
x86_64 execution probe. It passed checks under QEMU AArch64 and on the user's
RG34XXSP running KNULLI (kernel 4.9.170). Required glibc symbols are at most version 2.39; the reported handheld
uses glibc 2.40. Package SHA-256:

```text
2863c2b95adc5c627290bbf42a08abdfeabf7694ab57aab42a2c1464edf72be7
```

## Test Windows command execution

After the CPU diagnostic passes, extract
[school-tycoon-wine-scripts.tar.gz](releases/school-tycoon-wine-scripts.tar.gz)
into `/userdata/roms/ports/`. It adds [prepare-wine.sh](tools/prepare-wine.sh)
and [test-wine.sh](tools/test-wine.sh) to the existing
`school-tycoon-runtime-probe/` directory. Through SSH, run:

```sh
cd /userdata/roms/ports/school-tycoon-runtime-probe
sh prepare-wine.sh
sh test-wine.sh
```

The preparation script downloads the pinned Wine 11.19 AMD64 WoW64 build from
Kron4ek/Wine-Builds and verifies its publisher's SHA-256 before extraction. It
requires Internet access, curl, SHA-256 support, XZ-capable tar, and at least
2 GiB free. It writes only beneath the diagnostic directory and leaves an
existing Wine directory untouched. The upstream download is approximately
95 MiB and extracts to approximately 807 MiB.

The test creates a dedicated Wine prefix and runs 64-bit and 32-bit Windows
command shells. Each execution must exit successfully and print the exact
expected marker. It uses no display, game files, or Winetricks installation.
The first run can take several minutes; each command has a timeout. Background
Wine services in this test prefix are stopped on exit.

Share `logs/wine.txt` and, if needed, `logs/wine-64.txt` and `logs/wine-32.txt`.
These tests remain unverified on the device. A pass validates Windows CPU
execution only; Wine's display driver, Direct3D 8, audio, and gameplay require
separate tests.

## Findings and tools

- [Executable and CatCore assessment](docs/executable-assessment.md)
- [Device inventory, runtime approach, and validation results](docs/target-runtime.md)
- [Read-only device report](tools/device-report.sh)
- [CPU diagnostic script](tools/test-box64.sh)
- [Portable Wine preparation](tools/prepare-wine.sh)
- [Windows execution diagnostic](tools/test-wine.sh)
- [x86_64 probe source](tools/x86-probe.s)

Box64 upstream source is pinned to
`45f85efc52d7bd3a3c8642bfc5093ce7932f6c23`; see the diagnostic package's README
for build details. No original game executable or assets are distributed here.
