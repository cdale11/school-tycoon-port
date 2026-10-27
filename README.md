# School Tycoon for RG34XXSP

The user confirms working graphics, sound and faster gameplay on the handheld.
The latest update isolates cursor graphics state, keeps pointer movement inside
the game area and repairs menu observation across Wine startup. Cursor-box and
text-rendering symptoms still need handheld confirmation; shorter school-loading
times are not verified. See the [cursor repair and validation report](docs/cursor-controls-update.md)
and [audio validation](docs/audio-menu-update.md).
README edits remain excluded from startup checksums.

A compatibility launcher for the original School Tycoon (2004) on KNULLI Scarab
with PortMaster. **This is a handheld release candidate. A playable port on the
actual Mali device is not yet certified.**

## Download and use

[Fresh installation instructions](docs/fresh-install.md) ·
[Second audit and fresh emulator results](docs/fresh-validation.md)

The earlier native-media failure was traced to missing `.acm` codecs in the
32-bit prefix. This build repairs them and checks ADPCM support before launch.
Native media is downloaded and checksum-verified once on the device; proprietary
DLLs are never bundled. See the [current validation report](docs/audio-menu-update.md) for evidence and limits.

[Download school-tycoon-handheld.tar.gz](releases/school-tycoon-handheld.tar.gz).
Stop the game and extract it into `/userdata/roms/ports/`, replacing included
files. Remove an old `school-tycoon/cpu-mode.txt` interpreter override to use
the default dynarec/native-audio path. Keep existing game data, saves and the Wine prefix. The archive contains
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

On the verified original executable, D-pad selects buttons on Main, Play Select
and Credits; A clicks the selected button. The first direction focuses the first
button. Other screens retain pointer control. Hold L1 for precise pointer control
on any screen. This is a compatibility adapter, not native controller support in
the original game. A/B click left/right; X is
Space; Y is Escape; Start is Enter; R1 is Tab; L2/R2 are Page Up/Down.
**Fn/Guide + Start exits; Select + Start is the fallback. Save first.**
Physical Fn mapping and USB input on the handheld remain unverified.

Back up `school-tycoon/gamedata` and `school-tycoon/prefix-wine-10.0`, following
links when applicable. Retain any old runtime-probe directory still targeted
by prefix/runtime links. No reset is required. Conservative dynarec is now the game default. To fall back to interpretation,
put `interpreter` in `school-tycoon/cpu-mode.txt`; remove that file to restore
the default. Native audio requires dynarec; the interpreter fallback selects builtin media.
Put `builtin` in `audio-mode.txt` for an audio fallback while retaining dynarec.
Prefix creation/repair still uses interpretation; desktop startup follows the selected CPU mode.

## Memory and diagnostics

The build includes optimized Box64, Cortex-A53-tuned native helpers, joined
vertex shaders, uniform-array fixes and corrected internal shader sources.
Heap arenas are limited to two; Wine rendering stays in the bridge's single
graphics context. The handheld uses native Mali EGL/GLES, with no software Mesa
bundled. Conservative dynarec accelerates the game CPU; forced multithreaded
rendering stays disabled. Select Low detail and 800x600 in-game when available.

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
game audio failed that earlier audit; the repaired build now passes the menu-music capture. This remains a release candidate.

- Actual handheld, earlier tests: Box64 execution, Wine interpreter Windows
  commands, native Mali EGL/GLES pixels and a Windows GDI window. The latest user test confirms that game graphics now appear on Mali,
  with improved animation/input speed but unwanted side bars and missing sound.
- Earlier ARM64 VM builds: original intro/menus, classroom placement and save, Day 3 with
  eight students. Sustained scene runs stayed around 688–701 MiB game RSS with
  no process swap. This is not total device memory or a Mali memory estimate.
- Integration checks: controller motion/clicks/input release/Fn exit, launcher
  working directory and owned-process cleanup, Wine cache recovery, prefix
  preservation, bounded logging, ELF architecture and package checksums.
- Still requiring handheld confirmation: full-panel scaling and menu selection.
  The repaired audio path needs handheld confirmation; school-loading delays remain unmeasured. The current conservative dynarec fixture exits normally through the shortcut;
  older dynarec experiments exited 137. Weston previously showed a teardown assertion.

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
