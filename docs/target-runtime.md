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
