School Tycoon — experimental full-game launcher for KNULLI RG34XXSP

This launches the original game, not a diagnostic. It is NOT a verified playable
port. Latest device tests failed to create Direct3D 8; the early EGL initialization helper is
an unverified workaround on Mali. The verified CPU interpreter can be very slow.

INSTALL
Extract beside your other Ports, under /userdata/roms/ports/.
The archive contains "School Tycoon.sh" and the "school-tycoon" directory.
For the public GitHub package, copy the contents of your original ST folder to:
  /userdata/roms/ports/school-tycoon/gamedata/
SchoolTycoon.exe and Data must be directly inside gamedata.
The private archive already includes your supplied files; install it into a new
school-tycoon directory so existing settings or saves are not overwritten.
Refresh the KNULLI games list, then select School Tycoon under Ports.
You can also run: sh "/userdata/roms/ports/School Tycoon.sh"

The launcher reuses the previously installed Wine, Weston and Wine prefix when
available. Otherwise it downloads checksum-verified Wine 10.0 once (62 MiB) and
extracts the installed PortMaster Weston runtime. Allow 2 GiB free, or 3 GiB
when extracting the private archive. First Wine startup may take several minutes.
The private archive contains Wine already and needs no Wine download.

CONTROLS
D-pad: mouse pointer             A: left click (hold to drag)
B: right click                 X: Space
Y: Escape                      Start: Enter
L1 held: precision pointer     R1: Tab
L2 / R2: Page Up / Page Down
Select + Start: stop this game session (save in-game first)
USB mouse and keyboard can also be used.
Controller input is sent only to this session's X display.

SAVES
Game-directory saves/settings remain in school-tycoon/gamedata; Windows user
files remain in school-tycoon/prefix-wine-10.0. If the launcher reused the old
probe prefix, that path is a link: keep school-tycoon-runtime-probe until you
have backed up or moved that prefix. Back up both locations. No save reset or
settings.dat edits are performed. Use Low detail and 800x600 in game if it opens.
Actual saving, audio, shaders and gameplay on the handheld are not yet verified.

CPU
Default: interpreter, the only mode with successful Windows exits on this device.
Optional: write the single word dynarec into school-tycoon/cpu-mode.txt to try a
more conservative dynamic-recompiler configuration. Earlier recompiler runs
were killed with status 137; this new configuration is unverified. Remove that
file to restore the interpreter. No automatic CPU retries or diagnostic runs.

LOGS
school-tycoon/logs/launcher.txt, game.txt, game-wine.txt,
game-wine.stdout.txt, game-weston.txt and controls.txt are captured automatically.
A process starting or exiting successfully does not demonstrate playable gameplay.

THIRD-PARTY COMPONENTS
Box64 MIT, pinned 45f85efc52d7bd3a3c8642bfc5093ce7932f6c23:
  https://github.com/ptitSeb/box64/tree/45f85efc52d7bd3a3c8642bfc5093ce7932f6c23
Wine 10.0 LGPL, unmodified Kron4ek AMD64 WoW64 build:
  https://github.com/Kron4ek/Wine-Builds/releases/tag/10.0
  https://gitlab.winehq.org/wine/wine/-/tree/wine-10.0
The launcher verifies Wine's published SHA256 before downloading/extracting.
Debian libgcc_s.so.1: copyright, license and source provenance under runtime/x86_64.
Weston/Crusty/GL4ES are reused from your installed PortMaster runtime.
Original game files remain proprietary and are omitted from GitHub packages.
