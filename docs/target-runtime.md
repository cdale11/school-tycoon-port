# RG34XXSP runtime feasibility

## Reported device

User ran `tools/device-report.sh` on the target:

- Architecture: `aarch64`; kernel: `4.9.170`.
- KNULLI identification: Batocera.linux 42, Buildroot version ID 2024.11,
  OS date `20260511`; `OS_VERSION` empty.
- RAM: 996,484 kB total; 654,248 kB available at measurement.
- Swap: 498,240 kB total. Available swap and performance were not measured.
- Mali device: `/dev/mali0`.
- EGL, GLES1, GLES2 and libmali libraries found under `/usr/lib`.
- No Wine, Box86, Box64, GLX/EGL/Vulkan diagnostic commands, Xorg, or Xwayland
  found on the SSH command PATH. This does not prove they are absent in bundles.
- No desktop GL or Vulkan libraries found within the diagnostic search scope.
  Actual driver capabilities remain unqueried.
- PortMaster: `/userdata/system/.local/share/PortMaster`.
- Bundled runtimes found: Mono, Python, Weston 0.2, and Zulu Java 11.

Follow-up user report identifies glibc 2.40, with both
`/lib/ld-linux-aarch64.so.1` and `/lib/ld-linux-armhf.so.3` present.
The armhf loader alone does not establish a complete 32-bit library stack.

## Installed-game inventory

The user's Linux file listing contains 2,477 entries. It shows only two DLLs,
`nvlinker.dll` and `m4d.dll`, both already supplied in CatCore. There are three
executables: the supplied `SchoolTycoon.exe`, `School.exe` (53,248 bytes), and
`SchoolUpdater.exe` (204,800 bytes).

Configuration includes 180 INI files (195,436 bytes), 28 ENG files (241,470 bytes),
root `settings.dat` (1,792 bytes), and `version.bin` (1,138 bytes). The data tree
also includes `main.bin` and `inicache.bin`. Preserve all of these during a launch
test; their actual roles need inspection before selecting runtime overrides.

The asset tree contains 414 MOB files, 309 MESH files, and extensive textures,
terrain, UI, sounds, and challenge saves. Music includes 23 SGT files, so sound
validation must cover the game's DirectMusic behavior as well as WAV playback.
The splash file `Data/UI/SHELL/SPLASH/tmovie.bin` alone is 36,905,666 bytes,
exceeding the attachment transfer limit. Whole-game upload requires splitting
the compressed archive rather than simply grouping all individual files under
32 MiB. No assets should be dropped just to make an archive fit.

## Configuration archive findings

The supplied configuration archive contains 217 regular files, totaling
1,649,948 uncompressed bytes. It was extracted outside the source checkout;
archive paths were checked and links rejected. No supplied executable was run.

- `Readme.doc` is RTF. Its performance advice specifies Low detail, High Color,
  800x600, Vertex Shading, and music volume zero. These are documented menu
  choices, not verified command-line flags or decoded binary settings.
- The same readme documents typing `safemouse` during gameplay to enable a
  software cursor, and warns that joystick calibration can cause camera drift.
- `settings.dat` is a 1,792-byte binary file. Its schema remains unknown; do not
  patch guessed offsets to force display settings.
- `Data/Language.ini` selects English (`eng`) with default currency/code page.
- `School.exe` is another PE32 x86 executable. Its strings include
  `SchoolUpdater.exe`, `CreateProcessA`, `MoveFileA`, `DeleteFileA`, and a
  `New%s` path template. These suggest launcher/update behavior, not another
  engine. Disassembly or execution is required to establish exact behavior.
  The initial runtime test should invoke `SchoolTycoon.exe` from the game root.
- `SchoolTycoon.aud` begins with a RIFF/DMAP header, consistent with DirectMusic
  audio-path data. The `.spt` file is also binary; avoid treating it as a script.
- No obvious fullscreen/resolution/advertising switch was found in the supplied
  INI/ENG files. This search does not prove that runtime switches do not exist.

Assets still needed for an actual launch include the UI and splash files,
terrain, meshes/materials, textures, fonts, and sounds. Configuration inspection
alone cannot validate the engine or a compatibility runtime.

## Investigated route

Candidate, pending target verification:

`SchoolTycoon.exe -> x86_64 Wine WoW64 under Box64 -> WineD3D -> GL4ES -> EGL/GLES`

Wine WoW64 can support this 32-bit Windows executable without installing a full
32-bit ARM userspace. This requires an appropriate WoW64 build, not arbitrary
Wine64. An alternative is 32-bit Wine with Box32 support or Box86 plus armhf
libraries; those prerequisites have not been established on this firmware.

The display connection needed by Wine, a compatible GL4ES build, the native
loader/libc, and shader support are outstanding prerequisites. Installed Weston
alone does not demonstrate that Wine's window driver works. No Windows runtime
has yet been installed on or executed by the target.

Do not select DXVK as the default: no usable Vulkan driver is demonstrated.
Do not equate a GL4ES presence or GL version override with successful WineD3D
rendering. Box64's Wine documentation explicitly warns of poor WineD3D
performance for 32-bit applications under x86_64 Wine WoW64. About 1 GB RAM is
an additional constraint; actual game startup and scenario performance must be
measured rather than estimated as playable.

## Upstream evidence

- [PortMaster KNULLI GL helper](https://github.com/PortsMaster/PortMaster-GUI/blob/61a48652ebfdaae1d8c1231070ca5e546c3f4f01/PortMaster/libgl_knulli.txt):
  supports a port-local `gl4es.aarch64`/`gl4es` directory; selects GLES2 and
  changes its backend when `/dev/dri/card0` is absent.
- [Box64 Wine documentation](https://github.com/ptitSeb/box64/blob/45f85efc52d7bd3a3c8642bfc5093ce7932f6c23/docs/WINE.md):
  documents Wine variants, WoW64, Box32, and the WineD3D performance limitation.
- PortMaster-Runtime HEAD `e50188bbd8049e58c69b9e3016323add2e369c2e` contains
  only a README. Its release asset listing could not be retrieved (HTTP 503).
  Consequently the complete available runtime catalog has not been verified;
  the report above describes only the device's observed installed bundles.

These sources were read in the cloud workspace. They are evidence of candidate
support mechanisms, not a tested School Tycoon port.

## Next inputs and validation gates

1. Obtain the installed game's complete file listing and configuration files.
   Preserve original assets and directory layout. The initial full archive
   exceeded the cloud attachment transfer limit; use archives below 32 MiB.
2. Identify a target-compatible runtime bundle/sysroot, including loader/libc
   and display dependencies, before compiling or shipping binaries.
3. Verify x86 execution, Wine window creation, and an actual D3D8 draw on the
   handheld before claiming graphics readiness.
4. Launch with the complete asset tree, capture logs, and test menus, scenario
   simulation, sound, saves, and controller-to-mouse input.

The current result includes a static compatibility assessment, device inventory,
and the successful handheld CPU probe documented below.
No game launch, graphics test, performance measurement, or playable build has
been completed on the handheld.

## Complete assets and first runtime build

All eleven supplied chunks (00 through 10) were reassembled in numeric order.
The compressed archive is 320,460,571 bytes; SHA-256:
`2c680fb71e8fa58b18eeb74f4302e32176360e5ed69707ee25453830082e5a63`.
Gzip/TAR reading succeeded; all member paths were checked and non-file/directory
entries rejected before extraction outside the checkout. It contains 2,476 files
totaling 511,559,562 bytes. Every listed game file matches its path and size;
the only omitted listing entry is the listing file itself, `t.txt`.
The executable hash matches the previously inspected upload.

A Wine 11.19 AMD64 WoW64 build from Kron4ek/Wine-Builds was downloaded and
verified against the publisher's SHA-256 manifest. Archive SHA-256:
`a1fe858b45d12c9b16b91c7601d9b934f68430f7b6b95e240212ab6e2a198d8a`.
Wine's 64-bit command shell could list the game executable. Game startup failed
before initialization. A separate 32-bit command probe also failed, and a minimal
Linux i386 ELF was rejected with Exec format error. The cloud host therefore
cannot provide a native x86 WoW64 baseline; these results do not establish a
School Tycoon application defect. No game menu or D3D draw was observed.

Box64 at the pinned upstream commit above was cross-compiled with GCC 14.2.0,
generic ARM64 dynamic recompilation enabled, Release configuration, Box32 off.
Its loader is `/lib/ld-linux-aarch64.so.1`; direct dependencies are libc, libm,
and libresolv. Its highest referenced glibc symbol version is GLIBC_2.39,
below the reported target glibc 2.40. This ABI inspection is not a device test.

Under QEMU AArch64 with the cross-toolchain ARM libraries, Box64 successfully:

- Printed its version.
- Executed the host's x86_64 `/bin/echo` with the expected output.
- Executed `tools/x86-probe.s`, verifying multiplication, conditional branching,
  a Linux stdout write, and exit status zero.

The generated `school-tycoon-runtime-probe.tar.gz` package includes Box64,
its MIT license, the x86 probe, a readme, and `tools/test-box64.sh`. It is a CPU
diagnostic, not a PortMaster game launcher. At this stage, target execution was
pending; the subsequent user result is recorded below. Windows WoW64,
display integration, Direct3D 8, audio, input, and performance remain outstanding.

## Handheld CPU diagnostic result and Windows test

The user's device log now confirms the Box64 version command and execution probe
passed, ending with `CPU diagnostic completed successfully.` Box64 identified
four Cortex-A53 cores, a 4,096-byte page size, and a 39-bit address space. The
static-binary compatibility warning did not prevent this specific probe from
passing. This result does not exercise libc wrappers, 32-bit Windows code,
graphics, or game execution.

`tools/prepare-wine.sh` downloads the checksum-pinned Wine build above beside
the existing CPU diagnostic. `tools/test-wine.sh` creates an isolated prefix
and tests the shipped x86_64 and i386 Windows command-shell executables. It
checks both exit status and exact stdout markers, records individual logs,
disables automatic Mono/Gecko installation, and uses timeouts. It stops only
Wine services associated with its own prefix. It deliberately tests without
a display connection; a pass cannot establish rendering support.

Shell syntax was checked in the cloud. Archive checksums and the referenced
Windows executables were verified locally. Actual preparation and Windows
execution on the target are still pending; they cannot be claimed as tested
from the earlier CPU probe. The next requested target artifact is `logs/wine.txt`.

## First Windows attempt: missing x86_64 libgcc

The user's Wine log stops during `wine --version`, before either Windows command
test executes. Box64 loads Wine's x86_64 Unix `ntdll.so` but cannot resolve
`libgcc_s.so.1`. Its ELF dependency table confirms that library is required.
This is an incomplete portable-runtime dependency, not evidence of a game bug.

The updated scripts archive includes unmodified x86_64 libgcc from Debian
`libgcc-s1` 14.2.0-19, obtained using APT's authenticated repository metadata.
SHA-256: `30c61ab012a4241bed033725a09b61f5fdd3bb7df95ee852d0b096520524c7af`.
Its highest referenced glibc symbol version is GLIBC_2.35. Copyright, GPLv3,
GCC Runtime Library Exception, and source-location notices accompany it.
The diagnostic now includes `runtime/x86_64` in `BOX64_LD_LIBRARY_PATH` and
reports a missing support library explicitly before starting Wine.

Under QEMU AArch64, the same Box64 binary successfully loaded this emulated
x86_64 library with immediate symbol resolution and found `_Unwind_RaiseException`
and `_Unwind_Backtrace`. This validates loading under emulation, not exception
unwinding or Windows execution on the device. Extract the updated scripts archive
over the diagnostic folder and rerun `test-wine.sh`; retain the installed Wine
tree and prefix. A target retry is pending.

## Second Windows attempt: Wine 11.19 allocation assertion

The user's subsequent logs confirm the x86_64 libgcc now loads and Wine prints
its version. Both command-shell attempts then abort at `virtual.c:278`, inside
`anon_mmap_alloc`, on `!(size & host_page_mask)`. Neither expected marker was
printed; Windows execution remains unverified.

Inspection of upstream Wine 11.19 `dlls/ntdll/unix/virtual.c` shows that
`virtual_init` sizes its page-protection directory from the detected host address
limit and passes that size directly to the page-alignment-asserting allocator.
On smaller address spaces that table can be smaller than a 4 KiB page. The device
reports a 39-bit address space, and the abort dump includes a 64-byte argument.
This strongly indicates an incompatibility in this runtime's initialization,
although the exact caller and limit computation have not been traced on target.
Do not infer a game defect or disable the allocator assertion to bypass it.

Stable Wine 10.0 uses `alloc_virtual_heap` for the combined initialization tables
and lacks that assertion in `anon_mmap_alloc`. The diagnostic has therefore been
updated to the unmodified `wine-10.0-amd64-wow64` build, using a new
`prefix-wine-10.0` and leaving Wine 11.19 and its prefix untouched. It uses
`WINEARCH=win64`, with the build's new WoW64 support supplying 32-bit Windows
execution. This is a source-informed compatibility workaround, not a verified fix.

Archive SHA-256, checked against the publisher's release manifest:
`aeebbbf239e548f0136f1cd72a2e109dd9a572a8f703da3c09fa40d74d5c255f`.
Archive size: 65,074,156 bytes; extracted size approximately 645 MiB. The archive
and required Windows command-shell paths were checked in the cloud. Device
execution and game compatibility remain pending.

The Wine 10.0 x86_64 command shell was executed natively on the cloud machine:
it exited zero and printed the exact `ST-WINE64-PASS` marker. This verifies the
build's 64-bit headless command path, not Box64 execution or its 32-bit path on
the handheld. The diagnostic now captures stdout and stderr separately and
disables informational Box64 logging during each command, so asynchronous
warnings cannot merge into the success marker and cause a false failure.

Source references:

- https://github.com/wine-mirror/wine/blob/wine-11.19/dlls/ntdll/unix/virtual.c
- https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/ntdll/unix/virtual.c
- https://github.com/Kron4ek/Wine-Builds/releases/tag/10.0

## Third Windows attempt: markers printed, exit status 137

The next user upload contains exact `ST-WINE64-PASS` and `ST-WINE32-PASS`
stdout lines. Thus both command-shell executables reached Windows command
execution under Box64/Wine 10.0 on the handheld. Both runner statuses are 137,
and stderr ends with `Killed`. The test correctly reports failure despite the
markers; clean process termination is not established. Exit 137 can result from
a timeout or another SIGKILL source, including OOM; the uploaded logs do not
establish which. No allocation assertion from the Wine 11.19 run appears here.

The updated runner records elapsed time against a 120-second timeout with an
explicit TERM signal and 5-second kill grace period. It records memory/swap
counters and available kernel OOM messages (which may include older events),
reports marker execution separately from exit status, and stops only its own
Wine-prefix services between tests. Headless window-driver errors cannot alone
explain this shutdown failure; a graphics capability claim remains unwarranted.

Two documented Box64 settings can now be tested via script arguments:

- `strongmem`: dynamic recompilation enabled with
  `BOX64_DYNAREC_STRONGMEM=2`, adding memory barriers including SIMD writes.
- `interpreter`: `BOX64_DYNAREC=0`, to compare against dynamic recompilation.

These are diagnostic hypotheses, not confirmed fixes. Both modes successfully
executed an x86_64 command under QEMU AArch64 in the cloud. The runner's timeout
semantics were checked with a command that printed its marker then remained
running: the timeout returned nonzero (124), so a marker cannot falsely turn
that case into a passing test. Actual Wine execution with these modes on the
handheld is pending. Existing Wine files and the prefix are preserved.

## Fourth Windows attempt: interpreter passes

The next user logs confirm interpreter-mode Windows 64-bit and 32-bit commands
both printed their expected markers and exited zero. Elapsed times were 105 and
106 seconds, respectively. The aggregate log ends with `Windows CPU diagnostic
completed successfully`. This establishes a working Windows CPU baseline under
Box64's interpreter on the actual handheld, not usable gameplay performance.

Strong-memory mode still exited 137 after 18 seconds (64-bit) and 12 seconds
(32-bit), well below the 120-second timeout. Both printed their markers before
being killed. These were not the runner's deadline kills. Available memory
afterwards was approximately 372–386 MB, versus 655–671 MB in interpreter mode.
The supplied OOM-report sections are empty; there is no evidence establishing
an OOM cause. The responsible kill mechanism and dynamic-recompiler behavior
remain unexplained. Interpreter mode will be the baseline for display work;
the faster dynamic-recompiler route still needs investigation for gameplay.

## Native graphics and Weston investigation

The official PortMaster catalog now resolves Westonpack 0.2's ARM64 runtime to:
https://github.com/PortsMaster/PortMaster-New/releases/download/2025-07-24_0745/weston_pkg_0.2.squashfs

Its 55,398,400-byte download was verified against the catalog's published MD5
`0a761c28877625861d9c9c4326664428` without bypassing TLS or integrity checks.
The locally computed SHA-256 is
`0bccaeebc11296bb1fe4278c9677cb82965c5a04bcd63745fa09d78228bc1a93`.
The runtime includes Xwayland, ARM64 X11 libraries, and multiple GL4ES/crusty
graphics paths. Presence alone does not prove successful Wine rendering.
Its upstream wrapper also performs device/global-process setup and dumps the
environment; it has been inspected but not executed by this project. A final
launcher must provide appropriately scoped startup and logging.

`tools/native-egl-probe.c` dynamically loads the installed EGL/GLES2 driver,
creates a 16×16 RGBA8 pbuffer/context, clears it to a known color, and checks
the readback pixel against RGBA 64/128/191/255 with a small RGB tolerance and
no GL error. It reports the driver and API strings and releases the context.
It requires no game assets or graphics development headers on the device.

Both native host and ARM64 builds compiled with warnings treated as errors.
The native host build passed under Mesa llvmpipe in surfaceless mode, reading
the exact expected pixel. The ARM64 binary imports glibc symbols through 2.34.
Actual target graphics execution is pending. `tools/test-graphics.sh` records
the native probe's status and the installed Weston image's checksum and relevant
file listing. It creates no visible window and does not launch the Weston
wrapper. Its result cannot establish GLX, Direct3D 8, or game readiness.

## Target native graphics result and isolated window test

The user's native graphics log passes with exit status zero and exact pixel
RGBA 64/128/191/255, GL error zero. It reports ARM EGL 1.4, Mali-G31,
OpenGL ES 3.2, and GLSL ES 3.20. The installed Weston image's size and SHA-256
match the inspected official image. This validates native EGL/GLES rendering
on the actual device; desktop GL/GLX and Direct3D remain unverified.

The next diagnostic is `tools/test-window.sh`. It verifies and extracts the
existing image into the diagnostic's runtime directory, starts a scoped
headless/pixman Weston compositor, and waits up to 30 seconds for its X socket.
It refuses an existing X0 socket or lock, provides private Wayland runtime and
module paths, and disables Wine's Wayland driver so the probe tests X11.
It does not execute the upstream wrapper or perform that wrapper's global
process kills, device-node creation, or environment dump. Only the compositor
PID it starts and services in the diagnostic's own Wine prefix are stopped.

`tools/windows-window-probe.c` is compiled as a 32-bit Windows GUI executable
with imports restricted to KERNEL32, USER32, and GDI32. It registers a window
class, creates and shows a window on the headless desktop, handles WM_PAINT,
fills the client area with a known color, and reads back that GDI pixel. Only a
matching pixel writes `ST-WINDOW-PASS`; failed APIs or readback produce a failure
marker and nonzero exit. The runner removes only the previous generated result
in its own logs directory and requires this run's marker plus exit zero.

Both 32-bit and 64-bit builds compiled with warnings treated as errors and no
C runtime linkage. A 64-bit build of the same source ran under native Wine 10.0
with Xvfb in the cloud, produced the marker, and exited zero. The ARM64 Weston
version command under QEMU was blocked by an absent cloud ARM libevdev; the
target's system library availability has not been inferred from that result.
Shell syntax and the pinned runtime's executable/module paths were checked.
The actual 32-bit probe and target compositor startup remain pending.

This is a headless Windows/X11/GDI test, not a physical-screen preview or a
Direct3D test. The next target artifacts are `logs/window.txt` and
`logs/window-weston.txt`.

## First window attempt: absent X11 socket directory

The target log confirms Weston loaded the headless backend, Pixman renderer,
kiosk shell, and Xwayland module. Xwayland then failed to bind
`/tmp/.X11-unix/X0` with No such file or directory. The missing parent directory
was an omitted launcher prerequisite. No Windows window probe ran. The later
linked-list assertion occurred during compositor cleanup after that failure;
it has not been diagnosed as an independent blocker.

The launcher now creates `/tmp/.X11-unix` with mode 1777 only if absent. It
preserves existing directories and permissions, rejects a symlink or non-directory,
and retains the guard against taking over X0 sockets or locks. This is temporary
X11 socket infrastructure, not firmware configuration. Shell syntax was checked;
target startup must be retried before claiming display readiness.

## Successful target window test and Direct3D 8 candidate

The following user logs confirm Weston/Xwayland startup, the 32-bit Windows
window probe's marker, pixel validation, and exit zero after 139 seconds. The
window/X11/GDI test passed on the target. Xwayland fell back to software
composition, consistent with this headless test. The missing Vulkan library
does not invalidate GDI success or establish Vulkan support. Fontconfig had
no default configuration; the runner now creates a local configuration using
Wine/runtime fonts and a cache inside the diagnostic directory.

Weston again asserts in `linked_list_remove` during teardown after its X server
and seat client disconnect. This is a reproducible runtime cleanup limitation,
not a failed Windows pixel test. It remains unresolved; do not describe the
complete compositor lifecycle as clean or the game environment as ready.

`tools/windows-d3d8-probe.c` loads `d3d8.dll` and resolves `Direct3DCreate8`,
creates a windowed HAL device with software vertex processing and a lockable
X8R8G8B8 backbuffer, clears to a known color, reads it through LockRect, and
presents. Only a matching color and successful calls produce `ST-D3D8-PASS`.
Failure markers include the stage and HRESULT. Both PE32 and PE32+ builds
compiled with warnings treated as errors and no C runtime dependency. The
64-bit variant passed natively under Wine 10.0/Xvfb/Mesa llvmpipe in the cloud,
with exit zero. That does not validate the target's 32-bit renderer path.

The runner's `d3d8` mode adds GL4ES GLX pass-through and the runtime's Crusty GLX
preload only to the Wine client process. It supplies the system EGL/SDL paths,
Mali SDL backend, and upstream-style Crusty bridge settings. `core4es` selects
the alternative provided by the same runtime. The compositor continues to use
headless/Pixman and separate Xwayland libraries. Those client bridge settings
can present a colored SDL surface on the physical framebuffer and have not
yet been validated on the device. No upstream wrapper, global process kills,
or fake DRM nodes are introduced.

The runner still requires the newly generated marker and zero exit; the
300-second timeout does not count as success. The next target artifacts are
`logs/d3d8.txt` and `logs/d3d8-weston.txt`, plus the corresponding Core4ES logs
if that alternative executes. Game startup, shader behavior, audio, input, and
performance remain outstanding even if this simple Direct3D probe passes.

## Full-game launcher and latest D3D8 results (2026-10-13)

The uploaded GL4ES run exited 1: SDL reported "Can't load EGL/GL library on
window creation", the Mali driver reported a tracking-handle mapping failure,
and Wine failed Direct3DCreate8. The Core4ES run also exited 1: Wine could not
resolve glAccum and disabled OpenGL. No Direct3D device passed on the handheld.

The full-game launcher selects GL4ES, forces Crusty's GLES 2.0 request (the
installed binary defaults to GLES 3.2), and explicitly sets native EGL/GLES
library paths for SDL and GL4ES. This is a candidate workaround, not a confirmed
fix for the driver mapping failure. No Core4ES fallback is offered because that
run established a missing API. The actual game launches from gamedata without
a diagnostic timeout or a probe gate. Nothing patches game settings or DRM.

Controller support is a native SDL2/XTest helper scoped to this session's X11
display. It handles reconnects and releases held inputs on exit. Select+Start
returns to the launcher, whose cleanup terminates only its owned controller,
compositor and Wine prefix. The helper's host build passed a real SDL virtual
controller/Xvfb integration check: pointer movement, both clicks, release, exit.
The session shell passed a separate stand-in runtime integration check for asset
cwd, arguments, native GLES environment, game status propagation and cleanup.
ARM64 compilation passed with warnings treated as errors (glibc <=2.34).
These checks do not validate handheld gameplay or physical button mappings.

A fresh cloud attempt with the original executable, Wine 10.0, and Xvfb exited
139 after entering experimental WoW64 mode. There is still no successful game
startup baseline. Interpreter mode remains the default; conservative dynarec
is opt-in and unverified. Original game assets are included only in a private
workspace archive for their uploader, never in the GitHub package.

## Targeted native EGL initialization update

The full-game device run reached DirectMusic and created the Session Manager,
then failed exactly as the earlier probe: tracking-handle mapping failure,
SDL window creation failure, and Direct3DCreate8 initialization failure. Waiting
cannot recover this renderer initialization error. Asset placement was valid.

The installed Crusty binary's exported load_lib_local entry point loads EGL and
SDL through fixed /tmp/CRUSTY_LIB*64.so aliases; create_symlink leaves an existing
alias unchanged. The new MIT helper interposes this exported loader through its
PLT and loads the requested native absolute paths directly, avoiding those
persistent aliases. The binary's AArch64 JUMP_SLOT relocation for load_lib_local
was inspected to establish that this hook is applicable.

The helper also recognizes the game/probe argument in /proc/self/cmdline and
initializes EGL_DEFAULT_DISPLAY in a native ELF constructor, before Box64 starts
Wine and Wine reserves its Windows virtual address ranges. It retains the EGL
handle for the process lifetime. This tests the hypothesis that delayed Mali
tracking-handle allocation conflicts with those mappings; the supplied logs do
not contain mmap errno/address evidence proving that cause. A driver failure
at this earlier stage exits status 1 with an explicit message instead of launching
into a black screen. Non-target Wine services skip early GPU initialization.

Cloud checks passed using real Mesa EGL/GLES: early initialization and native
pixel readback, target filtering, direct library-loader API, and missing-library
failure/exit. The launcher integration check verifies that only the Wine child
gets the two-library preload (helper before Crusty) and the game target selector,
as well as cwd/status/cleanup. Native ARM64 compilation passed with warnings
as errors; the helper requires glibc at most 2.34. These are not handheld or
Direct3D/gameplay passes. Device verification remains required, with the helper's
status captured automatically in game-wine.txt.

## Repeated-download installation failure

The latest launcher log shows the Wine download and SHA256 verification both
succeeded, followed by "mv: cannot overwrite non-directory .../runtime/
wine-10.0-amd64-wow64 with directory .../.wine-stage...". The old installer
checked only -e, so a dangling symbolic link was treated as an absent path,
but remained in place and blocked the final directory move. Cleanup then removed
the downloaded archive, repeating the same download/install failure at each launch.
This run never reached Wine/game execution; it does not test the graphics update.

The installer and Ports entry now remove only dangling runtime links, preserve
real non-directory files and reject them before download, and validate existing
Wine installations before reusing them. The installer stores checksum-verified
archives in runtime/downloads and retains them on extraction failure. Stale
prefix links are moved to a dated backup link rather than deleted; no actual
prefix or save directory is removed. Restoring a missing target may recover its
old prefix, but no saves can be recovered from a deleted target by this update.

The recovery integration check passed with the actual pinned Wine 10.0 archive
and a curl stub that rejects every network call: normal-file preservation,
broken runtime link removal, failed extraction retaining the verified cache,
successful real extraction/install, second-launch reuse without downloading,
and Ports-entry recovery preserving a dangling prefix link. These validate
installer behavior; they are not gameplay or handheld graphics passes.

## Interrupted prefix / missing WoW64 module

The restarted handheld run exited 53: init_wow64 could not load
C:\windows\system32\wow64.dll (c0000135). Native EGL priming passed, but game
startup and rendering were never reached. This is not a renderer pass. The
five-minute timer is irrelevant to a run that has already exited.

A new prepare-prefix.sh restores only missing/empty wow64.dll, wow64cpu.dll,
and wow64win.dll from the installed Wine 10.0 x86_64-windows directory. Existing
empty files/links are preserved under dated backup names, and existing nonempty
modules and saves remain intact. It then runs the 64-bit wineboot.exe -u in
interpreter mode, without display variables, LD_PRELOAD or the graphics target
selector. A pending marker preserves retries when update fails or is interrupted.
Successful prefixes skip repeated repair. Launcher errors and setup logs are
captured before the graphical session is started.

Validation: the recovery integration test passed missing-module restoration,
existing-module/save preservation, clean bootstrap environment, failure retry,
and subsequent reuse. The same updater command completed successfully with the
real Wine 10.0 runtime against a fresh cloud-host prefix, restoring the WoW64
files; expected headless window warnings were present. Native-host Wine bootstrap
success does not validate Box64/handheld setup speed or 32-bit game rendering.
The runtime-install/Ports-entry integration check also passed after adding the
new bootstrap stage. No game or save files are present in the update archive.
