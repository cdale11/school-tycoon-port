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
