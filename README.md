# School Tycoon for RG34XXSP

An experimental launcher for the original School Tycoon (2004) on KNULLI Scarab.
**A playable port is not yet verified.** The latest handheld tests fail during
Direct3D 8 initialization. This package launches the actual game with a revised
GLES configuration; that workaround has not been tested on the handheld.

## Install and launch

1. Extract [school-tycoon-play.tar.gz](releases/school-tycoon-play.tar.gz) into
   `/userdata/roms/ports/`.
2. Copy the contents of your original `ST` folder into
   `/userdata/roms/ports/school-tycoon/gamedata/`, so `SchoolTycoon.exe` and `Data`
   are directly inside it.
3. Refresh the KNULLI games list and launch **School Tycoon** under Ports.

Existing Wine/Weston installations and the test Wine prefix are reused when
available. Otherwise the launcher downloads checksum-verified Wine 10.0 and
extracts the installed PortMaster Weston image. Keep at least 2 GiB free.
No diagnostic commands are required. First startup may take several minutes.

D-pad moves the mouse; A/B click left/right; hold L1 for precision; X is Space,
Y is Escape, Start is Enter, R1 is Tab, and L2/R2 are Page Up/Down.
Select + Start stops this game session. Save in-game before using that shortcut.
A native SDL controller helper sends input to the private X display.

Game files, settings and game-directory saves remain in `school-tycoon/gamedata`.
Windows user data remains in `school-tycoon/prefix-wine-10.0`, which can be a link
to the earlier diagnostic prefix. Back up both and keep any linked directory.
Logs are captured automatically in `school-tycoon/logs`.
See [installation and controls](packaging/PLAY-README.txt) for details.

## What is verified

- Handheld: Box64 CPU execution, Wine 10.0 Windows commands in interpreter mode,
  native Mali EGL/GLES pixel rendering, and a Windows GDI window.
- Cloud: controller movement/clicks/release/exit using real SDL virtual input and
  X11; launcher command, working directory, exit status and owned-process cleanup
  using stand-in runtimes; ARM64 helper build (glibc 2.34 maximum requirement).
- **Failed on handheld:** GL4ES could not create its SDL/Mali window; Core4ES
  lacked `glAccum`, causing Wine to disable OpenGL. Both failed Direct3DCreate8.
- **Unverified:** revised native GLES library selection, game rendering, sound,
  saves, physical controls and usable performance. A cloud attempt to run the
  original 32-bit executable under native Wine 10.0 exited with segmentation
  fault; it does not supply a successful gameplay baseline.

Interpreter mode is the only CPU baseline that passed on the handheld and can
be extremely slow. An optional conservative dynarec mode is included, but earlier
recompiler runs ended with status 137 for an unresolved reason. Weston also has
an unresolved teardown assertion. These are material remaining porting blockers.

## Development

- [Runtime findings](docs/target-runtime.md)
- [Executable assessment](docs/executable-assessment.md)
- [Earlier diagnostics](docs/diagnostic-history.md)
- [Controller source](tools/st-controls.c)
- [Session launcher](tools/test-window.sh)

Run `python3 tests/launcher-integration.py` for session orchestration checks.
For controls, compile `cc -shared -fPIC tools/st-controls.c -ldl -o /tmp/st-controls.so`
and run `DISPLAY=:93 python3 tests/controls-integration.py /tmp/st-controls.so`
with Xvfb on display 93 and native SDL2, X11 and Xtst libraries installed.
The source is compiled for ARM64 using the existing GCC 14 cross toolchain.

No original game executable or assets are distributed in this repository.
