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
