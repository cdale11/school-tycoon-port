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
x86_64 execution probe. It passed checks under QEMU AArch64; device testing is
pending. Required glibc symbols are at most version 2.39; the reported handheld
uses glibc 2.40. Package SHA-256:

```text
2863c2b95adc5c627290bbf42a08abdfeabf7694ab57aab42a2c1464edf72be7
```

## Findings and tools

- [Executable and CatCore assessment](docs/executable-assessment.md)
- [Device inventory, runtime approach, and validation results](docs/target-runtime.md)
- [Read-only device report](tools/device-report.sh)
- [CPU diagnostic script](tools/test-box64.sh)
- [x86_64 probe source](tools/x86-probe.s)

Box64 upstream source is pinned to
`45f85efc52d7bd3a3c8642bfc5093ce7932f6c23`; see the diagnostic package's README
for build details. No original game executable or assets are distributed here.
