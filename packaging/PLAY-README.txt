School Tycoon — RG34XXSP / KNULLI Scarab handheld candidate

INSTALL OR UPDATE
Stop School Tycoon. Extract school-tycoon-handheld.tar.gz into
/userdata/roms/ports/, replacing the included files. Do not delete your existing
gamedata, prefix-wine-10.0 or linked runtime-probe directory. No assets, saves,
settings or Wine prefix are included in this archive. Remove an old cpu-mode.txt
interpreter override to use the default dynarec/native-audio path.

For a fresh reset, see docs/fresh-install.md in the repository. Move both old
school-tycoon and school-tycoon-runtime-probe directories out of Ports first
so no existing prefix/runtime is reused. Back up saves before removing files.

For a new installation, copy the contents of your original ST folder into
/userdata/roms/ports/school-tycoon/gamedata/. SchoolTycoon.exe and Data must be
directly inside gamedata. PortMaster must already be installed. Allow 2 GiB free.
Refresh the KNULLI games list, then launch School Tycoon under Ports.
Alternatively: sh "/userdata/roms/ports/School Tycoon.sh"
Wine 10.0 is reused if installed; otherwise one checksum-verified download is
cached. The PortMaster Weston runtime is extracted locally. First prefix setup
may take several minutes; initial registration allows up to 600 seconds. GUI desktop readiness allows up to 180 seconds and
blocks game launch on failure. PE32 media registration/COM readiness is checked
separately before desktop readiness, allowing up to 180 seconds once per helper
build/prefix. The separate desktop helper exits when the game window appears.
Logs identify setup, readiness and game launch phases.
Concurrent launches are rejected automatically; no lock file deletion is needed.
A controller-helper failure stops the game and is recorded in controls/game logs.
Native graphics failure also stops the session rather than leaving a black screen.
The native SDL/EGL surface, shader compiler and a tiny draw/readback now run
before Wine's address reservations. Initial graphics logs are preserved separately.
Repeated GPU allocation/framebuffer failures stop the session automatically.
The user confirms graphics on the handheld. The new scaling/audio/menu changes
still need physical-device confirmation. No KNULLI Quick Resume or boot-game settings are changed.
When Quick Resume is enabled, KNULLI itself can arm a boot target before starting
the port. See docs/handheld-display-audit.md for the audit and emulator limits.
This is a compatibility launcher for the original game, not a rewritten engine.

CONTROLS
D-pad: select buttons on recognized Main, Play Select and Credits screens;
first direction focuses the first button. A selects. Other screens: pointer.
Requires the verified original executable; unavailable observation falls back
to pointer control. This adapter reads game state without changing game memory.
A: left click/hold to drag; B: right click.
L1 held: precise pointer; X: Space; Y: Escape; Start: Enter.
R1: Tab; L2/R2: Page Up/Down. USB input on the handheld is unverified.
Fn/Guide + Start exits this game session; Select + Start is the fallback.
Save first. Simulated SDL exit is tested; physical Fn mapping is not certified.

MEMORY AND GRAPHICS
Optimized Box64 with conservative game dynarec, Cortex-A53 native helpers, bounded heap
arenas and single-context rendering are the default. Native Mali EGL/GLES is
selected on the handheld. No software Mesa renderer is bundled. Select Low detail
and 800x600 in-game when available. No settings or save is edited automatically.
The game and desktop use conservative dynarec; prefix repair uses interpretation.
Put interpreter in cpu-mode.txt for the slower fallback; remove it to restore
the default. Forced multithreaded rendering remains disabled.
The 800x600 game image stretches across the entire 720x480 output, without side bars.
Keep 800x600 selected; stretching and menu navigation target that resolution.
Symbols are stripped to reduce storage, not claimed as a RAM saving.

SAVES
Back up gamedata and prefix-wine-10.0 (follow its link if applicable). In-game
saves normally live in gamedata/Data/SAVED_GAMES. Never delete a linked probe
directory until its prefix and Wine runtime have been backed up/moved.

AUDIO
Wine uses the firmware native audio backend; volume/mute/output still apply.
Raise Sound Effects and Music in the game Settings if they are at minimum.
The package enables native DirectMusic and restores Wine's missing Microsoft
ADPCM codec. Original menu music now passes a non-silent, unclipped VM capture.
This does not certify the handheld speaker path or every sound in the game.
The first update may download the checksum-pinned Microsoft runtime once; the
DLLs are extracted locally, never bundled with the package. Later starts reuse it.
The launcher preserves the existing PulseAudio connection. Prefix readiness
checks both media classes and ADPCM decoding support. Keep dynarec enabled for
native audio; the interpreter fallback selects builtin media because native
DirectSound distorted the interpreter test. To keep dynarec but disable native
media, put builtin in audio-mode.txt. Remove that file to restore native media.
The port does not implement rumble.

DIAGNOSTICS
Logs live in school-tycoon/logs/. launcher.txt: installer and package checks.
system.txt: firmware, build identity, memory/storage, native libraries and audio
inventory. prefix-media.txt/stdout.txt: PE32 media registration/class readiness.
game.txt: launch phases, owned PIDs and exit status. desktop.txt: Wine GUI readiness.
controls.txt: controller
mapping/connection. game-wine.txt and game-wine.stdout.txt: bounded output with
a .previous segment, at most 256 KiB per segment. memory.csv: last hour of
30-second process RSS/swap/thread and system-memory samples. game-weston.txt:
private display startup/teardown. A preceding launcher/session attempt is retained.
For graphics warnings and CPU/library startup details, put the single digit 1 in school-tycoon/debug.txt. Remove
it to restore normal error-only Wine logging. Logs do not include environment
dumps or game assets. Send the whole logs directory if startup fails.
No fixed black-screen timer proves readiness; consult the launch phase and logs.

VALIDATION LIMITS
The latest ARM64 VM test renders Main/Settings across the full panel, opens
Settings using D-pad/A and captures unclipped original menu music. Earlier builds
reached a school, placed a classroom, saved and reached Day 3 with eight students.
Those older scene-memory figures do not validate native-audio school memory.
The new menu fixture used about 345 MiB game RSS, not total device memory.
The VM uses software GLES and a virtual audio sink; it cannot reproduce Mali,
physical speakers/buttons or real handheld loading speed. Intro skipping and a
school-loading speedup are not established. This remains a release candidate.
See docs/audio-menu-update.md in the repository for exact evidence and limits.
Licenses and source links are in school-tycoon/BUILD-NOTICES.txt.

README text files may be edited without failing the package integrity check.
Executable scripts and runtime binaries remain checksum verified.
