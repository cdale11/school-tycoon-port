# School Tycoon for RG34XXSP

Latest handheld logs showed a native Mali surface failure. The new startup candidate creates and verifies the native window before Wine mappings and exits cleanly on graphics failure. See the [display and Quick Resume audit](docs/handheld-display-audit.md). Physical display success remains unverified; this is still a development candidate.

A compatibility launcher for the original School Tycoon (2004) on KNULLI Scarab
with PortMaster. **This is a handheld release candidate. A playable port on the
actual Mali device is not yet certified.**

## Download and use

[Fresh installation instructions](docs/fresh-install.md) ·
[Second audit and fresh emulator results](docs/fresh-validation.md)

The second audit found unresolved original-game audio failures. The failed
native-media experiment is excluded from the default build. See the fresh
validation report for separate rendering, controls and audio results.

[Download school-tycoon-handheld.tar.gz](releases/school-tycoon-handheld.tar.gz).
Stop the game and extract it into `/userdata/roms/ports/`, replacing included
files. Keep existing game data, saves and the Wine prefix. The archive contains
no proprietary game assets, saves, settings, prefix, emulator or diagnostic probes.

For a new installation, copy the contents of your original `ST` folder into
`/userdata/roms/ports/school-tycoon/gamedata/`. `SchoolTycoon.exe` and `Data`
must be directly inside it. Refresh the games list and launch **School Tycoon**
under Ports. PortMaster must already be installed; allow 2 GiB free. Wine 10.0
and Weston are reused where available. Otherwise Wine is downloaded once with
checksum verification and the installed PortMaster Weston image is extracted.
First prefix setup may take several minutes; subsequent launches reuse it.
A temporary helper checks the private Wine desktop before game startup and exits
when the original game window opens. Desktop failure/timeout is logged separately.

D-pad moves the pointer; A/B click left/right; L1 gives precise movement; X is
Space; Y is Escape; Start is Enter; R1 is Tab; L2/R2 are Page Up/Down.
**Fn/Guide + Start exits; Select + Start is the fallback. Save first.**
Physical Fn mapping and USB input on the handheld remain unverified.

Back up `school-tycoon/gamedata` and `school-tycoon/prefix-wine-10.0`, following
links when applicable. Retain any old runtime-probe directory still targeted
by prefix/runtime links. No reset is required. If you opted into experimental
dynarec previously, remove `cpu-mode.txt` to use the stable interpreter default.

## Memory and diagnostics

The build includes optimized Box64, Cortex-A53-tuned native helpers, joined
vertex shaders, uniform-array fixes and corrected internal shader sources.
Heap arenas are limited to two; Wine rendering stays in the bridge's single
graphics context. The handheld uses native Mali EGL/GLES, with no software Mesa
bundled. Forced multithreaded rendering and the unverified CPU recompiler are
not enabled by default. Select Low detail and 800x600 in-game when available.

`school-tycoon/logs/` contains firmware/build identity, launch phases, controller
connection, process exit status, display logs, bounded Wine output and the last
hour of memory samples. Each Wine output segment is limited to 256 KiB, retaining
one previous segment. Put `1` in `school-tycoon/debug.txt` for additional graphics
warnings and CPU/library startup details, then remove it to return to normal logging. Send the whole logs directory
when reporting a failure. No logging can anticipate or identify every fault.

[Full installation, controls and troubleshooting](packaging/PLAY-README.txt).
[Archive checksums](releases/SHA256SUMS). Earlier diagnostic archives are legacy;
use the handheld archive for the current game build.

## Evidence and remaining limits

The current fresh VM audit reaches a new Farmland school and writes a new save;
simulated Fn/Guide + Start exits the actual game. Scene RSS is about 611–613 MiB,
with an observed 626 MiB process high-water value and no process swap. Original
game audio fails the capture check. This remains a development candidate.

- Actual handheld, earlier tests: Box64 execution, Wine interpreter Windows
  commands, native Mali EGL/GLES pixels and a Windows GDI window. Earlier
  Direct3D game startup failed; the current graphics fixes have not been
  confirmed on Mali.
- Earlier ARM64 VM builds: original intro/menus, classroom placement and save, Day 3 with
  eight students. Sustained scene runs stayed around 688–701 MiB game RSS with
  no process swap. This is not total device memory or a Mali memory estimate.
- Integration checks: controller motion/clicks/input release/Fn exit, launcher
  working directory and owned-process cleanup, Wine cache recovery, prefix
  preservation, bounded logging, ELF architecture and package checksums.
- Unverified on handheld: rendering/scaling, sound, physical controls and
  playable speed. Interpreter emulation can be slow. Dynarec previously exited
  137, and Weston has shown a teardown assertion.

The VM matches Cortex-A53, four cores, ARM64, 4 KiB pages, 39-bit address space,
4.9.170 kernel and approximately the reported RAM/swap. It substitutes a virtual
board and software graphics. QEMU cannot reproduce H700/Mali-G31, its closed
driver, panel, audio or buttons; this is not a complete or 1:1 handheld emulator.

## Development

[Cloud setup and regression checks](docs/cloud-setup.md).

- [Earlier package validation and limits](docs/final-validation.md)
- [Build inputs, package contents and memory settings](docs/handheld-build.md)
- [ARM64 VM fixtures and limitations](docs/emulator.md)
- [Runtime findings](docs/target-runtime.md)
- [Executable assessment](docs/executable-assessment.md)
- [Earlier diagnostics](docs/diagnostic-history.md)

`tools/build-handheld.py` builds the asset-free archive from pinned runtime inputs
and the ARM64 cross compiler. See the build document for arguments. Run
`python3 tests/launcher-integration.py`, `python3 tests/log-integration.py`, `python3 tests/entry-integration.py`,
`python3 tests/prefix-repair-integration.py` and the documented native/VM probes.
The repository does not distribute the original executable or game assets.

The subsequent [code audit](docs/code-audit.md) fixes shader-cache retention,
EGL ownership, shutdown and concurrent-launch handling. Historical diagnostic
archives remain available in Git history; the current release directory contains
the handheld package and compatibility aliases.
