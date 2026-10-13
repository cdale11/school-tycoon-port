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

The preparation script downloads the pinned stable Wine 10.0 AMD64 WoW64 build from
Kron4ek/Wine-Builds and verifies its publisher's SHA-256 before extraction. It
requires Internet access, curl, SHA-256 support, XZ-capable tar, and at least
2 GiB free. It writes only beneath the diagnostic directory and leaves an
existing Wine directory untouched. The upstream download is approximately
62 MiB and extracts to approximately 645 MiB.

The scripts archive also includes x86_64 `libgcc_s.so.1` from Debian
`libgcc-s1` 14.2.0-19, with copyright and license notices. Box64 needs this
emulated library to load Wine's `ntdll.so`; an ARM system copy does not replace
it. The diagnostic adds the local library directory to `BOX64_LD_LIBRARY_PATH`.
Wine 11.19 aborted on an internal memory-allocation assertion on the target.
The updated diagnostic uses Wine 10.0, whose virtual-memory initialization uses
a different allocation path. Extract the updated scripts archive and run both
preparation and testing again. Wine 10.0 uses its own directory and prefix;
the earlier Wine 11.19 installation is retained. This workaround still needs
verification on the handheld.

The test creates a dedicated Wine prefix and runs 64-bit and 32-bit Windows
command shells. Each execution must exit successfully and print the exact
expected marker. It uses no display, game files, or Winetricks installation.
The first run can take several minutes; each command has a timeout. Background
Wine services in this test prefix are stopped on exit.

Share `logs/wine.txt` and, if needed, `logs/wine-64.txt` and `logs/wine-32.txt`.
These tests remain unverified on the device. A pass validates Windows CPU
execution only; Wine's display driver, Direct3D 8, audio, and gameplay require
separate tests.

The latest device logs contain both Windows markers, but both commands ended
with exit status 137. Windows code execution is demonstrated; successful
shutdown is still unresolved. To investigate, extract the updated scripts
archive (no Wine download needed) and run:

```sh
sh test-wine.sh strongmem || sh test-wine.sh interpreter
```

The first mode enables Box64's documented strong memory emulation. If it fails,
the second disables dynamic recompilation to isolate a possible recompiler
problem. Each mode tests both command shells with a 120-second timeout and a
5-second termination grace period. Share `logs/wine-strongmem.txt` and, if run,
`logs/wine-interpreter.txt`. The logs distinguish a printed marker from a clean
exit and include elapsed time, memory counters, and available kernel OOM reports.
Interpreter mode is diagnostic, not a proposed gameplay performance setting.

The follow-up device run passed both Windows commands in interpreter mode with
exit status zero (105–106 seconds each). Strong-memory mode still ended with
137 after 12–18 seconds, before the timeout. No OOM report identified the cause.
Interpreter mode is now the verified Windows CPU baseline; graphics and usable
game performance remain unverified.

## Test native graphics

Extract the latest scripts archive into `/userdata/roms/ports/`, then run:

```sh
cd /userdata/roms/ports/school-tycoon-runtime-probe
sh test-graphics.sh
```

Share `logs/graphics.txt`, including failures. This adds a native ARM64 EGL/GLES2
probe, tests a 16×16 off-screen pbuffer with pixel readback, and records graphics
library and Weston-runtime metadata. It does not start Weston or modify firmware.
No new Wine download or repeat of the Windows CPU tests is needed.

A native graphics pass does not establish Wine GLX or Direct3D 8 compatibility.
The probe's source is [native-egl-probe.c](tools/native-egl-probe.c); its rendering
logic passed on the cloud's Mesa software renderer. Actual Mali-driver validation
is pending.

The handheld native probe passed: ARM EGL 1.4, Mali-G31 OpenGL ES 3.2, exact
RGBA 64/128/191/255 readback, and no GL error. The installed Weston image also
matches the inspected official runtime. Wine graphics remain untested.

## Test a Windows window

Extract the latest scripts archive into `/userdata/roms/ports/`, then run:

```sh
cd /userdata/roms/ports/school-tycoon-runtime-probe
sh test-window.sh
```

Share `logs/window.txt` and `logs/window-weston.txt`. This extracts the already
installed, checksum-verified Weston runtime locally (approximately 133 MiB),
starts a private headless desktop with Xwayland, and runs a 32-bit Windows probe
in the verified interpreter mode. The probe creates and paints a window, reads
back its GDI pixel color, writes a marker file, and must exit zero to pass.
Allow several minutes for Wine startup; the Windows command has a 240-second
timeout. No physical window appears on the handheld screen in this test.

The launcher refuses to take over an existing X0 socket/lock and stops only its
own compositor PID and test-prefix Wine services. It does not invoke the
upstream Weston wrapper, create fake DRM devices, or change firmware. It does
not test Direct3D 8, GLX rendering, original-game startup, or gameplay speed.

Source: [test-window.sh](tools/test-window.sh) and
[windows-window-probe.c](tools/windows-window-probe.c). A 64-bit build of the
same probe passed under Wine 10.0 and Xvfb in the cloud. The shipped 32-bit
build and the ARM64 compositor integration still require target validation.

The subsequent handheld run passed the 32-bit X11/GDI probe with exit zero,
the expected marker, and a matching pixel after 139 seconds. Weston still
asserts during teardown; that runtime cleanup defect remains recorded. The
launcher now supplies a local font configuration for the fontconfig warnings.

## Test Direct3D 8

Extract the latest scripts archive and run:

```sh
cd /userdata/roms/ports/school-tycoon-runtime-probe
sh test-window.sh d3d8 || sh test-window.sh core4es
```

Share `logs/d3d8.txt` and `logs/d3d8-weston.txt`. If the alternative ran, also
share `logs/d3d8-core4es.txt` and `logs/d3d8-core4es-weston.txt`.

The 32-bit probe loads Wine's Direct3D 8 DLL, creates a HAL device, clears a
lockable backbuffer, checks the pixel color, and presents it. The client uses
the installed runtime's Crusty GLX bridge and GL4ES (or Core4ES) over Mali/GLES.
Those Wine bridge combinations are candidates, not verified target solutions.
A colored surface may briefly appear on the physical screen in these modes.
Allow up to five minutes per command. No game assets or runtime downloads are
needed. GDI mode remains a purely headless test.

Source: [windows-d3d8-probe.c](tools/windows-d3d8-probe.c). Both architectures
compiled successfully; the 64-bit version passed under native Wine 10.0 with
Mesa llvmpipe in the cloud. That validates the probe logic, not ARM/Mali support,
game shaders, sound, controls, or performance.

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
