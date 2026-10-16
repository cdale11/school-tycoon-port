# Handheld display and Quick Resume audit — 15 October 2026

The submitted handheld logs identify a **native surface creation failure**, not
an unfinished Wine installation. Prefix CPU/media checks and the private Wine
desktop completed. Crusty then reported:

```
Could not create SDL Window: mali-fbdev: Can't create EGL window surface
Could not create OpenGL Context: Invalid window
Failed to get a GL context for adapter
```

The game stayed alive at roughly 79 MiB RSS. Waiting longer does not repair that
failed graphics initialization. Missing Vulkan is not the selected GLES path's
failure. Weston is deliberately headless: SDL/Mali supplies the physical screen.
The readiness window in Weston therefore never proved physical presentation.

The startup system report showed CmaTotal=65536 KiB and CmaFree=2552 KiB despite
700484 KiB of general available RAM. Contiguous graphics allocations are a
separate constraint. This snapshot alone does **not** prove CMA exhaustion was
the failing allocation, and SDL's final error hides the original driver error.
The exact Mali error and native allocation behavior remain hardware-dependent.

## Changes

The preload now creates the **same pinned Crusty SDL window and GLES context**
before Wine's Windows address reservations, checks both the native context and
draw surface, and reuses them during GLX rendering. Previously only EGLInitialize
ran early; that did not allocate the actual surface. This addresses allocation
ordering as a candidate display fix; a physical Mali success is not established.
No vendor binary or firmware SDL library is patched.

Native failure now exits before Windows code. The launcher additionally detects
the observed Crusty/Wine failure messages and stops its captured children rather
than leaving a live invisible game. Loss of Weston also ends the session.
Download connection, total and stalled-transfer deadlines are bounded. Installer
signals now exit explicitly and remove only their staging directory. Logs include
framebuffer geometry, inherited SDL rotation/blitter hints, CMA samples and a
bounded kernel tail on graphics failure. No rotation or framebuffer setting is
changed by this update.

## Quick Resume investigation

Audited the Ports entry, run-game, Wine/prefix/media installers, telemetry,
controller helper, session lock and historical diagnostic scripts. None enables
or writes `global.quickresume`, `global.bootgame.cmd` or `global.bootgame.path`.
They do not source PortMaster control.txt, call batocera-settings, reboot the
device, kill EmulationStation or install boot-time services. Cleanup signals only
captured session children and the Wine server for the selected private prefix.

Upstream KNULLI EmulationStation source explains the indirect behavior:

- [FileData.cpp](https://github.com/knulli-cfw/batocera-emulationstation/blob/f7c5ae103815761ccd34bc55b021c6e150442c96/es-app/src/FileData.cpp):
  launchGame calls setQuickResume **before** running the port, then calls
  postLaunchConditionalClean after it returns, including nonzero exit status.
- [QuickResume.cpp](https://github.com/knulli-cfw/batocera-emulationstation/blob/f7c5ae103815761ccd34bc55b021c6e150442c96/es-app/src/QuickResume.cpp):
  an enabled global.quickresume writes bootgame.path/cmd. Cleanup clears them
  only when Quick Resume is enabled and `/var/run/shutdown.flag` is absent.
- [main.cpp](https://github.com/knulli-cfw/batocera-emulationstation/blob/f7c5ae103815761ccd34bc55b021c6e150442c96/es-app/src/main.cpp):
  startup runs an existing configured boot command, then conditional cleanup.

This is source-level evidence from the upstream `knulli` branch, not proof that
its exact commit matches the user's compiled Scarab frontend. The uploaded logs
do not contain the user's Quick Resume setting or demonstrate a reboot loop.

A hung port followed by shutdown can leave the frontend's resume target armed.
Returning normally on graphics failure allows its game-end cleanup to run.
The port deliberately **does not clear or restore these keys**, since that would
violate the requested no-modification rule and could overwrite a user setting.
It cannot prevent an enabled frontend from arming Quick Resume before the port
starts. A guarantee against that requires frontend support or the user disabling
Quick Resume / "Launch this game at startup" in KNULLI. No automatic configuration
edit or reboot workaround is installed.

## Emulator coverage

QEMU retains Cortex-A53 ×4, kernel 4.9.170, 39-bit VA, 4 KiB pages, glibc 2.40 and
1007 MiB configured RAM. The updated VM was rebooted and reports the requested **64 MiB CMA**
pool (`CmaTotal=65536 kB`, `MemTotal=996496 kB`) instead of the kernel's 16 MiB default. This models the budget, not Mali's
allocation use or the observed 2552 KiB free state. The profile records the real
failed-run CMA reading and explicitly identifies SDL mali-fbdev as unemulated.

Run `python3 tools/emulator/check-handheld-failures.py` alongside VM gameplay.
It exercises the actual preload's success/failure/target-selection paths, replays
the handheld error with a live fake Wine process through the real session shell,
and guards firmware-setting/reboot operations. The VM runner also rejects those
graphics errors instead of treating a living Wine process as success.

The actual ARM64 pinned Crusty/GL4ES framebuffer test passed with early native
initialization, including pixel readback and binding restoration. Software GLES
cannot validate `/dev/mali0`, ION allocations, physical display ownership or the
RG34XXSP SDL rotation backend. Therefore this is a tested startup correction and
candidate display fix, **not a certified working handheld release**.

## Delivered package and final run

The updated asset-free archive is 5,452,726 bytes, SHA256
`e6a5aad8272e5947cf7cabb8deefbfaf1943d7e72094cc2dcdb13c34f72a82ab`.
The two legacy release names contain the identical archive. Internal hashes and
archive paths were checked; no original assets or firmware-setting tools are bundled.

This exact archive was extracted over the existing private VM installation,
preserving Wine, the prefix, game files and the prior save. With the 64 MiB CMA
boot, it passed desktop readiness and rendered the original main menu. Menu RSS
was 226876 KiB (about 222 MiB), with zero process swap. This is a menu measurement,
not the memory needed by a loaded school. No new full-school or audio pass is
claimed for this update. The VM's null audio server stopped during this run;
audio remains unresolved as documented in fresh-validation.md.

An initial cold-desktop VM attempt exceeded the 180-second deadline before the
graphics patch was invoked. A diagnostic retry succeeded. The VM-only runner now
allows 600 seconds for cold services under TCG; the handheld keeps its separate
180-second desktop limit. This is a test-time allowance, not a claimed firmware fix.

Fourteen host regression suites passed, including surface failure, live-process
cleanup, installer cancellation, firmware-setting guards, prefix recovery, log
rotation, and existing rendering regressions. The actual ARM64 framebuffer probe
also passed during the candidate investigation.

The exact-archive run also passed simulated Guide/Fn + Start exit through the
unchanged SDL controller helper. Intentional termination returned status 143,
and no SchoolTycoon process remained. Physical button wiring is unverified.

## Follow-up: native shader and framebuffer failures

The next handheld run passed surface creation but repeatedly reported
`GL_OUT_OF_MEMORY`, `GL_INVALID_FRAMEBUFFER_OPERATION` and shader link failures.
Process RSS was around 130 MiB with no process swap; these errors do not prove
that the device exhausted its general-purpose RAM. Both rotating stderr logs
had lost the initial failure. Waiting longer cannot repair this rendering loop.

The new candidate exercises the native GLES3 shader compiler, a 2×2 framebuffer,
and a draw/pixel readback immediately after surface creation, before Wine reserves
its address space. Temporary objects are deleted and changed native GL state is
restored. This tests a possible lazy driver-allocation problem; the supplied logs
do **not** establish that cause. A failed warmup prevents game startup and records
the stage, GL error and compiler/linker information.

Two independent corrections apply regardless of that hypothesis:

- Preserve the first 64 KiB of each Wine stream in `game-wine.startup.txt` and
  `game-wine.stdout.startup.txt`, alongside bounded rotating tails.
- End the owned game session after repeated allocation **and** framebuffer
  failures (at least 16 of each in the current stderr log). Individual capability
  probe rejections do not trigger this guard. There is no automatic retry.

The VM runner now applies the same failure guard and preserves initial logs.
Native Mesa tests verify real shader compilation, drawing, readback and restoration
of deliberately altered GL state across three repetitions. The actual ARM64
warmup probe also passed. An ARM64 framebuffer test passed with both GL4ES ARM
workarounds forced on; they are therefore retained. These software tests cannot
reproduce the proprietary Mali driver, ION or physical display ownership.

The follow-up archive is 5,456,138 bytes, SHA256
`780c0d9798ecf3cbe86d3291a3d926bb8ecbe213cbc28d2a8f4774ab126e4c4f`.
Replace the complete package, including its manifest and build identifier; keep
existing assets, saves, Wine and the prefix. This remains an unconfirmed physical
GPU fix, not a certified playable release. Original-game audio remains unresolved.

The exact follow-up archive passed the private desktop gate, native warmup and
original main-menu rendering in the ARM64 VM with both ARM GL4ES framebuffer
workarounds enabled. The captured menu was visually checked. Menu RSS was
221164 KiB (about 216 MiB), with zero process swap. This is a menu test with
software GLES, not a new full-school, audio or physical-handheld pass.

Simulated Guide/Fn + Start also passed through the controller fixture with this
archive: intentional exit status 143 and no remaining SchoolTycoon process.
Physical button wiring remains unverified. All 14 host regression suites passed;
the additional real GLES3 state-restoration test passed separately.

## Latest physical run: warmup passed, rendering still failed

The supplied fresh logs show ARM GLES 3.2 returning the expected warmup pixel
(64,128,191,255) with GL error zero. Wine's private desktop also passed. During
subsequent WineD3D capability checks, GL4ES reported an ARB program link failure
with an empty info log, followed by custom program link failures and repeated
GL_OUT_OF_MEMORY / invalid-framebuffer errors. The launcher correctly exited
with phase `rendering-failed`. Warmup did not fix the physical rendering failure.

The captured kernel tail contains older suspend/network activity but no GPU
allocation failure identifying the cause. This does not rule out a driver
allocation failure, nor does GL_OUT_OF_MEMORY alone establish overall RAM
exhaustion. The Vulkan loader warning is not sufficient reason to install Vulkan:
the selected rendering path uses OpenGL through GL4ES.

`LIBGL_LOGSHADERERROR=1` now enables the pinned bridge's existing failure-only
shader source reporting. This is diagnostic coverage, not a rendering fix.
The emulator runner uses the same setting; software Mesa still cannot emulate
the proprietary Mali compiler, ION or physical display driver.

README text files (case-insensitive names ending in `readme.txt`) are excluded
from newly generated internal manifests and ignored in older manifests at
startup. Other entries remain verified. Tests confirm an edited README permits
startup while a corrupt runtime script is still rejected. No firmware settings,
game data, prefix or runtime cache are reset by this change.

The rebuilt documentation/diagnostic update is 5,456,378 bytes, SHA256
`b30d97b4d5190b3cf42401e5eae9fad5345a68d6fb47509e84c2d57b97d8eed8`. Entry, build-integrity, launcher and firmware-setting
regressions passed. Internal archive checksums were verified. No new full-game
or physical rendering success is claimed for this update.
