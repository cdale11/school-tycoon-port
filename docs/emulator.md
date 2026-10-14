# RG34XXSP cloud test environment

The emulator uses the user's measured device specifications in
`tools/emulator/rg34xxsp.json`: four Cortex-A53 cores, Linux 4.9.170,
4 KiB pages, 39-bit virtual addresses, glibc 2.40 and about 1 GiB RAM.
Guest memory is adjusted for the different kernel's reserved memory; measured
MemTotal and swap must be recorded for each run.

This is QEMU's `virt` board, **not an emulated H700 or Mali-G31**.
The publicly available RG34XXSP Gladiator II 20250813 filesystem uses
Buildroot 2024.11 and glibc 2.40, like the reported Scarab build, but is an
older firmware release. Do not call it an exact Scarab image. Software-rendered
graphics tests cannot certify the handheld's native Mali driver or performance.

## Prepared private inputs

Keep firmware images, original game assets, VM disks and runtime installations
outside this repository. The current cloud workspace retains them under
`/workspace/school-tycoon-analysis/emulator`.

- QEMU 10.0.13 from signature-verified Debian trixie packages, extracted locally.
- Linux tag `v4.9.170`, commit `ad387ec4bcdf087faaf084a3db8f681295463b01`.
  Build ARM64 defconfig with `CONFIG_ARM64_VA_BITS_39=y`,
  `CONFIG_ARM64_VA_BITS=39`, `CONFIG_HZ_100=y`, built-in virtio block/MMIO,
  9p/virtio, gzip squashfs, `CONFIG_HW_RANDOM=y` and
  `CONFIG_HW_RANDOM_VIRTIO=y`. GCC 14 build uses `HOSTCFLAGS=-fcommon`
  for the older bundled device-tree compiler and `KCFLAGS=-Wno-error`.
- Official KNULLI image downloaded from the `20250813` GitHub release;
  publisher SHA256 is recorded in the profile. Extract `boot/batocera` from
  the GPT `boot-resource` FAT partition (starting at sector 149504).
  This squashfs remains a read-only guest root disk.
- ARM64 static BusyBox in a gzip/newc initramfs with `tools/emulator/init`.
- Separate ext4 guest data disk. When creating it with current e2fsprogs,
  disable `metadata_csum_seed` and `orphan_file` for Linux 4.9 compatibility.
- Read-only host analysis share. Guest startup reads the private test script at
  `/userdata/host/emulator/shared/guest-test.sh`.

Run with `LD_LIBRARY_PATH` pointing to the extracted QEMU native libraries:

```sh
python3 tools/emulator/launch.py \
  --vm-dir /workspace/school-tycoon-analysis/emulator \
  --analysis-dir /workspace/school-tycoon-analysis \
  --qemu /workspace/school-tycoon-analysis/emulator/qemu/usr/bin/qemu-system-aarch64 \
  --results-dir /workspace/school-tycoon-analysis/emulator/results
```

The VM has no network device. Copy executable runtimes and game test data into
its ext4 disk before running them. An initial Box64 execution directly from
9p crashed before startup; the same binary copied to ext4 successfully printed
its version and ran Wine 10.0 `--version`. This was a VM storage issue.

## Evidence so far

The first VM boot reported Linux 4.9.170, four Cortex-A53 CPUs, 4096-byte pages,
and glibc 2.40. Box64 detected 39-bit addresses and Wine reported `wine-10.0`.
Fresh prefix creation and original-game execution are still under investigation.
No emulator gameplay or handheld gameplay pass is claimed.

## First original-game run (2026-10-13)

With executable runtimes copied to guest ext4 and native ARM64 Xvfb/Mesa
libraries installed, the following checks ran inside the Linux 4.9.170 VM:

- 32-bit Windows `cmd.exe`: exact `ST-PREFIX32-PASS` output and exit 0
  under Box64 interpreter mode.
- Native GLX: Mesa 25.0.7 llvmpipe / LLVM 19.1.7, OpenGL 4.5 and GLES 3.2.
- Original-game-era Direct3D 8 probe: `ST-D3D8-PASS`, exit 0, including
  clear, pixel readback and present.
- Original SchoolTycoon.exe plus the complete private asset set: created its
  session manager, initialized WineD3D and displayed the animated school intro.
  Main-menu interaction and gameplay are not yet verified.

These are actual ARM64 guest executions, not substituted Wine commands or host
x86 Wine tests. The initial VM used 1024 MiB RAM and a 62.5 MHz virtual counter;
its MemTotal was 1013908 KiB. The profile adjusts the next boot to 1007 MiB and
sets the reported 24 MHz counter. These adjustments still require a measured
reboot; do not imply the first run matched those values exactly.

Dynamic-recompiler command tests printed their markers but exited 137.
A syscall trace showed Wine's server polling process existence and issuing
SIGKILL during process shutdown. This demonstrates a server-initiated kill in
this VM, not a confirmed handheld OOM diagnosis. Wine 10.0's
`server/process.c:process_sigkill` applies a short shutdown grace period.
`BOX64_DYNAREC_INTERP_SIGNAL=1` did not fix that test. Disk-cache/shutdown
behavior is being investigated; no new recompiler default is established.

The X server must start with the correct native Mesa library and driver paths.
An earlier server created before those dependencies were installed had no GLX
visuals. Changing DISPLAY within an already-running Wine session also produced
an invalid comparison; restart that prefix's services when changing displays.

`tools/emulator/capture.c` captures the X11 root window as a private PPM frame.
Use the dedicated output share and `tools/emulator/capture.sh` described below.
The earlier raw-disk snapshot method was superseded after filesystem pauses
stalled Wine. Do not freeze live storage for screenshots. Stop the task VM
before inspecting or repairing its raw disk.

## School scene and remaining failures

The original game reached its main menu, accepted Play / Instant Action /
Farmland, loaded the level and rendered the 3D school grounds and HUD, including
buildings, trees, road, money and school statistics. Actual X11 frames were
captured privately. Menu pointer movement and some menu interactions worked;
building placement and save/load were not verified. The simulator ran with
software Mesa and Box64 interpreter mode, so these results do not predict
handheld frame rate.

Further CPU tests reproduced marker output followed by status 137 in both
32-bit and 64-bit Windows command programs. None of these experiments produced
a verified recompiler fix: disabling disk cache, interpreting signal handlers,
stronger flag/barrier settings, unmodified Box64 0.3.8, or an experimental
longer Wine server shutdown grace period. The experimental server is retained
only in the private analysis workspace and is not part of the launcher.

A separate GLES bridge attempt used the distributed GL4ES/Crusty/early-EGL
helper with software EGL/GLES and SDL's X11 backend. The isolated test exited 1
with CreateWindow failure (0x80070578), before a successful Direct3D test. This
is not a Mali-driver pass. Earlier CPU and bridge attempts shared a prefix;
those comparisons were invalid because desktop/service state could interfere.
Use a separate prefix for each graphics environment and stop only its owned
services between tests.

Native Mali rendering, a usable recompiler mode, reliable gameplay controls,
sound, saves and actual device performance remain unresolved. The handheld
launcher has not been changed on the strength of these failed experiments.

## Profile reboot validation

The checked-in launcher and rebuilt initramfs were booted with the final
profile. The guest reported four Cortex-A53 CPUs, Linux 4.9.170, glibc 2.40,
4096-byte pages, MemTotal **996496 KiB** (12 KiB above the device report), and
SwapTotal **498240 KiB**, matching the reported swap. Box64 measured the
**24.0 MHz** counter and detected **39-bit** addresses. Wine 10.0 `--version`
again exited zero. This validates the profile reboot, not a second gameplay run.
The VM starts with more available memory than the handheld's full frontend.

The retained private startup check is mirrored in
`tools/emulator/guest-check.sh`. It recreates only this VM's swap file and
checks runtime versions; never execute it on a handheld or another host.
Copy it into the read-only analysis share's `emulator/shared/guest-test.sh`
before preparing the initramfs. The default boot ends at a guest shell.

Captured frames and full traces remain private under the analysis workspace's
`emulator/output` directory. No original game or firmware assets are tracked.

## GL4ES bridge correction (2026-10-13)

Two launcher/bridge defects were reproduced:

- Box64 defaults its desktop libGL override to `SDL_VIDEO_GL_DRIVER`. The
  previous launcher set that to native GLES, so Wine bypassed GL4ES. It now
  sets `BOX64_LIBGL` explicitly to the selected desktop translation library.
- Bundled Crusty's `glXGetFBConfigs` allocates its opaque configuration inside
  the returned pointer array. Wine correctly frees that array and retains the
  configuration, producing a dangling handle. Later `GLX_DRAWABLE_TYPE` queries
  lose the window bit and Wine rejects the pixel format. The new native
  `st-glx-config.c` returns a separate caller-owned pointer array and keeps the
  original backing allocation alive for the process lifetime. It resolves the
  bridge through its explicit library handle: Box64's `RTLD_NEXT` resolution
  otherwise caused recursion in the initial experiment.

`tools/emulator/glx-config-probe.c` reproduced the lifetime bug using the real
ARM64 bridge: DRAWABLE-BEFORE=1 / AFTER=0, exit 4. With the helper it retained
BEFORE=1 / AFTER=1, exit 0. The host fixture also exercises both enumeration
functions after callers free their arrays; its uncorrected control fails.

For the VM's substituted SDL X11 backend, set `SDL_VIDEO_X11_FORCE_EGL=1`.
Without it SDL recursively used the desktop GLX path and failed context
creation. This is a VM setting; the handheld uses SDL's Mali backend.
`tools/emulator/graphics-env.sh` records the working VM library selection.

With the corrected native helper and interpreter/dynarec command experiments,
the actual PE32 Direct3D 8 probe wrote ST-D3D8-PASS: CreateDevice, Clear,
LockRect pixel readback and Present succeeded through GL4ES/GLES. Those sessions
still exited 137 during shutdown, including an interpreter client connected to
a recompiler server. Rendering success is distinct from clean process exit.
A fresh session with all Wine processes in interpreter mode then wrote the
marker again and exited 0. This is the clean GL4ES/GLES rendering baseline;
there was no kernel OOM report in the VM log.
The original game with this GLES bridge has not yet reached a verified scene;
the earlier original-game scene used desktop software Mesa.

The prepared guest can start its software X server with
`sh /userdata/host/emulator/shared/start-xvfb.sh :95` after copying the tracked
`tools/emulator/start-xvfb.sh` into the private analysis share. The script checks
QEMU's virt model, preserves existing display locks, provides xkbcomp through a
VM-only /usr/bin bind overlay, and starts Xvfb with its native Mesa paths.
It was run on display :96 and the native GLX/GLES capability query succeeded.
The original game test remains on :95. This script is not a handheld installer.

A stronger PE32 probe (`tools/windows-d3d8-texture-probe.c`) draws a managed
texture on a transformed quad and checks the center pixel against a different
color from the clear. It wrote ST-D3D8-TEXTURE-PASS and exited 0 through the
corrected GLES bridge. This establishes textured rendering as well as clear.
A native GLX program also produced a captured blue SDL window. However,
holding the Windows probe open after Present returned success still gave a
black captured frame, and the original game did not produce a verified visible
frame through this bridge. API success and offscreen readback do not establish
visible presentation. The update is still experimental.

Further presentation experiments remain private and have not changed the
handheld launcher: repeated presents, disabling automatic framebuffer swaps,
separating SDL and Wine X servers, single-threaded Wine rendering, and native
ES3 blit adapters. Only captured frames from live windows can establish an
onscreen result; a capture after its process exits is not a presentation test.

## Visible GLES presentation correction

The native framebuffer-copy helper (`tools/st-gl-present.c`) intercepts the
bridge's GLX procedure resolver using the bridge's explicit loaded handle.
Wine's GLES rendering completed into offscreen textures, but its original
framebuffer-copy route produced no visible output. The helper reads GL4ES's
framebuffer identifiers, binds those native GLES 3 read/draw targets, copies
through the driver's core operation, and restores the previous native bindings.
Other OpenGL procedures retain their original implementation and the bridge's
backend initialization still executes. The launcher requests ES3 while GL4ES
continues to translate desktop GL through its ES2 backend.

A held Windows D3D8 probe produced a captured blue rectangle after the corrected
copy. The original executable then displayed its complete main menu and accepted
Play, showing Challenge Mode / Instant Action / Load Game / Tutorial. Captured
frames remain private (`game-visible-focused.png`, `game-after-play.png`). This
is original-game execution through GL4ES/GLES, rather than the earlier desktop
software Mesa route. A VM focus correction was required: Xvfb had PointerRoot
focus, and explicitly focusing the Wine game window made its input active.
Level loading and interaction still require validation.

The final native helper was also tested with
`tools/emulator/native-blit-probe.c`: actual GL4ES texture/FBO creation, native
GLES copy, RGBA 64/128/191/255 readback, and unchanged native read/draw bindings.
It printed ST-NATIVE-BLIT-PASS and exited 0. No experimental native blit adapter
or verbose swap instrumentation is part of the handheld package.

For VM presentation, SDL can use a separate X server from Wine, approximating
Mali's independent display path. The private XOpenDisplay interposer used for
that substitution is a VM aid and is not shipped to the handheld.

## Reconnecting the cloud console

The cloud tool session registry changed while the VM process was still running.
The files survived but its original stdio console could no longer be resumed.
After restarting only the owned VM, `launch.py --socket-console` was boot-tested:
serial.sock and monitor.sock are private UNIX sockets in the VM directory.
Attach with `python3 tools/emulator/console.py /path/to/vm/serial.sock`.
The console client was stopped and reattached without stopping the VM, and the
monitor reported running. A stale socket path is rejected; check its owner and
stop only this task's VM before removing stale sockets. Do not start two QEMU
processes against the writable data disk.

## Cold-boot entropy (2026-10-13)

A restarted VM stalled in `SyS_getrandom` before the game initialized.
The Linux 4.9 CRNG had not initialized; this VM previously had no hardware
random source. A private fresh host-CSPRNG seed supplied through
`RNDADDENTROPY` unblocked that run, after which the original intro, main menu
and Farmland selection and level-loading screens rendered through the
corrected GLES path. The level test is still in progress; no GLES gameplay
pass is claimed.
This was an emulator startup fault, not evidence of a handheld Wine fault.

The launcher now requires built-in virtio random support and supplies a
`virtio-rng-device` backed by the host's `/dev/urandom`.
The Linux 4.9.170 virtio RNG driver sets its quality to 1000, allowing
the kernel to credit this trusted host entropy without a global override.
No deterministic seeds or game assets are included. A separate cold boot
using the rebuilt kernel, existing initramfs and this RNG device initialized
the CRNG at guest uptime 1.06 seconds. This isolated check attached no game
data disk, leaving the ongoing full-game test undisturbed.

## Original Farmland through GLES (2026-10-13)

The original executable and private assets reached the Farmland 3D scene
through the production framebuffer-copy helper and GLES 3 native context.
A captured frame shows the school building, terrain, road, fences, trees,
money/statistics HUD and principal panel. The top-right file control accepted
input and opened its save/exit buttons. The Save Game screen accepted the Save action and created
`Data/SAVED_GAMES/NewSave.sts` (about 198 KiB, with a Farmland scenario
header) in the private guest copy. Placement and reload
still require verification.

The full level exposed GLSL link failures (`setup_vs_output` unresolved),
matrix-uniform errors and draw errors that the earlier texture probe did not
exercise. A visible scene does not certify all models or effects. No complete
gameplay or handheld pass is claimed.

The cloud Box64 build also had empty release compiler flags, despite its
Release label. A separate unmodified build of the same revision now uses
explicit `-O2 -DNDEBUG`; it reports its version in the ARM64 guest. The optimized build ran Windows `cmd.exe` in interpreter mode and printed
`ST-OPT-CPU-PASS` with exit 0. Full-game validation remains pending. The existing
full-game run was left intact; the optimized binary is private and is not
included in a device update yet.

The first GLES school-scene run later exhausted guest memory and was killed
by the Linux OOM killer (status 137, game anonymous RSS about 897 MiB, with
swap in use). This is a failed sustained-run test even though the scene and
save succeeded. The emulator uses software Mesa, so its allocation pattern
is not a measurement of Mali memory use. Shader errors remain a separate
reproducible compatibility fault under investigation.

## Vertex output linking fix under test (2026-10-14)

Wine supplies `setup_vs_output` in a second vertex shader object. Desktop
OpenGL can link those objects; the pinned GLES bridge failed with an unresolved
function. `tools/st-gl-shader.c` joins the function with the main vertex source
for this specific two-object case. Native attachments are temporarily changed
for linking and then restored. Original sources, GL4ES attachment bookkeeping
and delete-pending shader ownership are preserved. Variants with additional
point-size uniforms fall back to the existing linker.

The actual ARM64 GL4ES/GLES test failed to link without the fix (exit 4). With
the fix it linked 64 times, retained all three original attachments including a
delete-pending output shader, and rendered the expected RGBA pixel
204/51/25/255 (exit 0). The existing native framebuffer-copy and binding
restoration probe also passed with the combined presentation/shader helper.
The full original-game test using the optimized interpreter runtime and this
linking approach is in progress. It recognizes the previous Farmland save;
reload and sustained gameplay are not yet verified.

Build the presentation helper for the target ABI from all four source files:

```sh
cc -O2 -fPIC -shared tools/st-gl-present.c tools/st-gl-shader.c tools/st-gl-uniform.c \
  tools/st-gl-source.c \
  -ldl -pthread -o libst-gl-present.so
```

For the unmodified Box64 revision recorded above, explicitly set
`-DCMAKE_BUILD_TYPE=Release '-DCMAKE_C_FLAGS_RELEASE=-O2 -DNDEBUG'`,
`-DARM64=ON -DARM_DYNAREC=ON -DLD80BITS=OFF -DNOALIGN=OFF` when configuring
the ARM64 build. Verify the generated interpreter flags include `-O2`; a
Release cache label alone is insufficient. The optimized binary requires
glibc 2.38 or newer, within the device's reported glibc 2.40 ABI.

The first full-game run with output-function joining still failed to link its
vertex/fragment interface: `_gl4es_TexCoord_0` had no matching vertex output.
It reloaded the saved terrain/HUD, but trees were missing, draw errors continued
and the game was again killed by OOM. This is not a successful reload/rendering
pass.

An expanded native test includes texture-coordinate outputs. The default
array conversion reproduces the link failure (exit 4). Enabling the documented
GL4ES option `LIBGL_NOTEXARRAY=1` passes all 64 links and renders the expected
colour (exit 0). The GL4ES launcher now selects this option alongside the joined
output function. A new full-game reload using both corrections is in progress.

## Capture without filesystem pauses (2026-10-14)

The earlier screenshot procedure froze `/userdata` while the raw disk was
read. Some freezes were left active too long, blocking Wine registry and cache
writes. Startup/shutdown timeouts and sustained-run timings overlapping those
pauses are unreliable and require rerunning. Native shader link errors and
pixel checks remain reproducible, but neither the freezes nor the observed
OOM events establish a handheld memory diagnosis.

The VM now has an optional dedicated writable output share selected by
`--results-dir`. The original analysis/game-input share stays read-only. The
initramfs mounts the output share at `/userdata/results`. Capture with:

```sh
bash /userdata/host/emulator/shared/capture.sh :96 frame.ppm
```

Copy `tools/emulator/capture.sh` into that private shared-script directory first.
After the command reports completion, read the corresponding host result file
directly. Do not freeze live guest storage or inspect its raw disk for ordinary
screenshots. Only use raw-disk tools after stopping this task's VM. Keep high-rate
game logs on guest ext4; copy selected closed files or log tails to the output
share for inspection.

A rebuilt initramfs and cold boot validated the output mount and a complete
800x600 RGB capture visible on the host. The guest reported MemTotal 996496 KiB
and SwapTotal 498240 KiB, with CRNG initialization at uptime 1.07 seconds.

The interactive rescue shell also inherited an initramfs-only PATH; startup
now includes `/usr/bin` and the firmware commands. The Xvfb overlay resolves
firmware links in their original location before binding the overlay and is
prepared only once per boot. Explicit coreutils and capture checks passed.

`tools/emulator/run-game.sh` supplies the full native graphics and x86 library
search paths, so the test does not depend on variables exported in an earlier
console session. Copy it and `graphics-env.sh` into the private shared-script
directory. The VM-only `sdl-display.c` helper routes SDL presentation to :96
while Wine input windows stay on :95; compile it with `cc -O2 -shared -fPIC
-ldl` and place it at `/userdata/libsdl-display.so`. This helper is not included
in handheld packages. Place the current combined presentation/shader helper at
`/userdata/libst-gl-present.so` before running the game test.

## Uniform-array corrections (2026-10-14)

With capture pauses removed and the launch environment explicit, the saved
scene reloaded and showed a character. Uniform-update errors continued and
memory was exhausted. The game was stopped; this remains a failed sustained
run, not a handheld memory measurement. Process maps showed growth in native
heap/worker arenas as well as the guest's Windows allocations.

`tools/st-gl-uniform.c` corrects two observed GL4ES cases using actual native
program reflection: an inactive array-element query returned a nonnegative
location, and a uniform write extending beyond the active array was rejected.
OpenGL requires excess array elements to be ignored; invalid scalar writes
remain errors. Array metadata is refreshed after linking and removed when the
program is deleted. GL4ES still performs the valid updates and maintains its
state cache.

The actual native test reproduces the bad lookup without the correction
(exit 7). With it, the inactive location is -1, oversized vec4/mat4 array writes
produce no error, an oversized scalar write produces GL_INVALID_OPERATION,
and the resulting pixel is 204/51/26/255 (exit 0). The existing native copy and
framebuffer-binding restoration test also passes with this helper. Full-game
and handheld validation of the added uniform correction remain pending.

Reference: [OpenGL glUniform semantics](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUniform.xhtml).

The uniform and framebuffer-copy probes also pass with Mesa Softpipe (native
GLES 3.1), providing a software path without LLVM compiler-worker allocations.
The VM environment now allows `GALLIUM_DRIVER=softpipe` for this comparison and
limits LLVM worker threads and glibc arenas in the default software test. These
settings apply only to the cloud emulator, not the handheld's Mali backend.
Use a distinct `ST_VM_LOG_PREFIX` for each full-game run to preserve its logs.
Full-game results for the added uniform correction remain pending.

## Saved-scene reload and native memory tracing (2026-10-14)

The combined shader/uniform fixes, optimized interpreter and Softpipe reached
the original saved Farmland scene. The captured frame includes the character,
trees, terrain and HUD. No `glUniform`, `walk_constant_heap`, or shader-link
failure was recorded in this run. This is a rendering improvement, not a
sustained-gameplay pass: memory kept growing after the scene appeared. The
game was deliberately killed when swap filled (RSS about 887 MiB, game swap
about 348 MiB); this termination was not attributed to the kernel OOM killer.

Two process maps show one large native anonymous region growing from about
331 MiB to 668 MiB, while the native heap and Windows regions stayed roughly
steady. Software-renderer allocation patterns cannot certify Mali memory use.

`alloc-trace.c` records live native allocations of at least 128 KiB, including
aligned allocation, with a bounded table and dropped-record counter. Ordinary
preload interposition does not reach Box64's executable-provided allocator.
`alloc-audit.c` therefore observes bindings to that allocator through glibc's
loader audit API, calling the exact original functions with unchanged arguments.
Only external bindings are observed; internal allocator calls and direct memory
mapping are outside its coverage. This diagnostic adds overhead and is excluded
from handheld packages and performance measurements.

The host integration test verifies both mechanisms against actual allocation,
resize, aligned allocation and release. A matching ARM64 VM audit fixture also
passed: 105 allocations, zero dropped records, and zero retained bytes after
release. Game allocation attribution is in progress. To enable the VM diagnostic,
build `alloc-audit.c` as a native shared library at
`/userdata/libst-alloc-audit.so`, then set `ST_VM_ALLOC_AUDIT=1` and a distinct
`ST_VM_LOG_PREFIX` when invoking `run-session.sh`.

`run-session.sh` snapshots the game and graphics environment scripts before
starting. Editing a shared shell script during an earlier active test changed
its unread portion and broke the final status write. That test's deliberate
kill status was preserved explicitly; future sessions execute immutable copies.
The wrapper removes only its own temporary script directory after completion.

Remaining work includes placement and another save/reload, sustained stability,
PortMaster Guide/Fn + Start exit handling, measured runtime optimization, and
closer firmware/device emulation. Native Mali and physical inputs remain
separate hardware checks.

The source controller helper now recognizes SDL Guide/Fn + Start, retains
Select + Start, and also recognizes PortMaster's `HOTKEY` setting (including
`l3`/`r3`). Real SDL virtual-controller/XTest checks passed all three tested
combinations (Guide, Select, configured L3). Start alone did not exit, and a held
mouse button was released on exit. The production launcher orchestration check
also passed; physical Fn and original-game exit remain unverified. Download
archives still contain the older controller until the runtime update is rebuilt.

The first allocator-audited level run also reached the school scene without the
earlier uniform/link failures, then exhausted memory and was deliberately
stopped. Tracked large readback buffers were released; large allocation records
alone did not explain all of the growth. The next run lowers
`ST_ALLOCATION_MIN_SIZE` to 4096 and uses
`ST_ALLOCATION_DUMP_INTERVAL=1000` to keep diagnostic output bounded. The
record table supports 262144 entries and reports dropped records. A host check
also verifies that the configured threshold actually filters allocations.

`map-trace.c` additionally records large mmap/munmap/mremap syscalls and native
link/frame addresses, including mappings that bypass ELF allocator bindings.
It uses Linux 4.9 arm64's syscall-entry/exit marker in x7, reads target registers
and frame words, and does not modify them. An actual VM fixture passed both
mapping/release capture and tracer detachment: the target continued and exited
zero after the tracer was stopped. This is diagnostic instrumentation, not a
handheld runtime component or a performance measurement.

VM game sessions now record their exact application PID in
`/userdata/<ST_VM_LOG_PREFIX>.pid.txt`. Optional `ST_VM_MAP_TRACE=1` starts the
mapping tracer for that owned process and records its separate exit status.
The tracer can instead be attached only during the scene-loading phase to
reduce startup overhead. Native Mali allocation and emulator timing still
require separate validation.

The 4 KiB allocator run retained about 458 MB at Mesa's `ralloc_size` call
site, in addition to its approximately 201 MB of fixed texture caches. Its
last summary recorded 337000 observed calls with zero dropped records. Debian
debug symbols matching the loaded Gallium build ID identify the first sample
as GLSL builtin-symbol/hash-table allocation; that first sample does not prove
which later compiler allocations account for the growth. The diagnostic now
samples fresh stacks each time a site's retained bytes grow by another 16 MiB.

An auxiliary display was accidentally run in the foreground of the old PID-1
rescue shell, which had no controlling terminal. The private VM was stopped,
its data disk backed up and its ext4 journal recovered before restart. The
rescue shell now runs as a child session with a controlling tty. Its PID,
session/process group and tty were verified, and Ctrl+C interrupted a native
sleep. Run persistent test displays in the background and wait for X11 readiness.

The ARM64 controller helper also passed the real SDL virtual-controller/XTest
checks inside the VM for Guide/Fn, Select and configured L3 plus Start, with
Start-alone and held-button-release checks. Physical inputs and full-game
exit are still separate checks.


## Internal shader variant correction under test (2026-10-14)

Fresh retained-allocation stacks reach Mesa shader linking from GL4ES's internal
`fpe_CustomShader`. The sampled run's stdout contains 877 unresolved
`setup_vs_output` failures. Earlier checks of Wine stderr missed these internal
failures; rendering a scene did not establish error-free or stable rendering.
The internal variants copy the first attached vertex source, which previously
still contained only the output-function declaration.

Our separate `st-gl-source.c` helper supplies the complete joined source when
the application selects its linked program. It preserves the application's
original source queries and restores that source before an application compile
or relink. It does not modify vendor source or private vendor structures.

The actual ARM64 alpha-test fixture reproduced the failure with the previous
helper (exit 6, erroneous RGBA 204/51/26/64), then passed with this correction
(exit 0, background RGBA 64/128/191/255). The fixture also verifies original
source queries, repeated linking with a delete-pending output shader, uniform
array writes and the scalar error check. Full original-game stability and
handheld Mali behavior remain unverified.


The extended alpha-test fixture also passed 16 relinks after program use,
checking the discarded pixel and original source each time. Native framebuffer
copy/binding restoration and uniform-array probes passed with the four-source
helper. The first two full-game attempts stopped before graphics initialization
with Wine's explorer-start failure; exit zero did not establish a game pass.
A plain GDI diagnostic loaded the X11 desktop; its exit 2 came from trying to
write its result in the read-only shell working directory. Startup diagnostics
are separate from the passing shader tests.

`sample-memory.sh PID filename` records process RSS, anonymous RSS, swap and
thread count alongside guest available memory every 30 seconds. It checks the
VM model and process start time, so PID reuse ends sampling. It only observes
the owned test and writes its bounded-rate log under `/userdata`.


A passive PE32 VM fixture, `windows-desktop-ready.c`, creates a hidden window,
writes `ST-DESKTOP-READY` in its writable working directory, and keeps the Wine
desktop alive in a message loop. It uses no graphics interposers and does not
send input. Its readiness marker was verified in the actual ARM64 VM. Launching
the original game after that marker progressed to DirectMusic/audio startup
without the earlier explorer failure. This is a testing workaround under
validation, not a handheld launcher requirement or a gameplay pass.

Build it with a PE32 MinGW compiler using `-Os -nostdlib -mwindows
-Wl,-e,_desktop_start`, linking `-lkernel32 -luser32`. Run it from a writable
private guest directory with the same Wine prefix/display as the game. Close
its window with WM_CLOSE, or stop that owned prefix's server during session
cleanup. Never infer readiness merely from the Wine process existing.


The corrected original-game run reloaded NewSave and rendered the Farmland
building, trees, character and HUD. Its paused scene remained alive across the
work interruption. Samples from 14:32–16:21 UTC stayed near 687–690 MiB RSS;
the last ten minutes ranged from 689540 to 689552 KiB, with no process swap.
Guest swap use settled near 11 MiB. GL4ES stdout contains zero internal FPE
link failures. This validates sustained rendering of a paused scene; it does
not yet validate active simulation, placement or handheld performance.
Native XTest held mouse clicks also opened Play and Load Game and started the
save, confirming the controller helper's input mechanism reaches this game.


The ARM64 SDL virtual-controller fixture subsequently ran while the original
game was alive. Guide/Fn + Start returned success from the controller helper;
a watcher using the launcher's exit decision stopped only that owned game PID
and its Wine prefix. `ST-GAME-EXIT-PASS` was recorded, and the immutable game
session preserved status 143 from the intentional stop. Start alone did not
exit, and held mouse buttons were released. This verifies the simulated
controller/cleanup path with the original game; physical buttons and the
complete handheld launcher/display path remain unverified.

`verify-controller-exit.sh LOG_PREFIX FIXTURE_PY HELPER_SO` repeats this private
VM check for a currently running owned test. It rejects finished log sessions
and unrelated process names. The observer uses the supplied ARM64 helper and
real SDL virtual-controller/XTest fixture, not mocked controller functions.

Placement remains unverified. In the separate-display 3D test, pointer motion
and building previews work, but native clicks, Wine SendInput and posted clicks
did not activate placement. A read-only probe confirmed matching foreground
and engine window handles. A synchronous click timed out, although the engine's
mouse-button state showed that its down event eventually arrived. All test
presses were cleared before the owned prefix was stopped. The next differential
test uses `ST_VM_BACKEND_DISPLAY=:95` to place SDL presentation and Wine input
on the same X display; the existing separate-display default stays :96.
