# ARM64 handheld test fixture

This QEMU guest tests ABI/runtime behavior and original-game graphics with
software GLES. **It is not a complete RG34XXSP or a 1:1 Scarab replica.**
QEMU does not emulate the H700 board, Mali-G31/libmali, panel, audio or physical
controls. Performance and native Mali allocations cannot be predicted from TCG.

## Profile and private inputs

`tools/emulator/rg34xxsp.json` records the supplied device report: Cortex-A53
quad core, ARM64, kernel 4.9.170, glibc 2.40, 4 KiB pages, 39-bit addresses and
24 MHz counter. QEMU RAM is 1007 MiB to approximate reported usable memory.
The measured guest MemTotal was 996496 KiB versus device 996484 KiB; swap
was configured as 498240 KiB. Boot measurements remain required for new guests.

The root filesystem is the publicly available KNULLI Gladiator II 20250813
Buildroot 2024.11/glibc 2.40 image, not the exact Scarab release. Firmware/kernel
hashes and substitutions are recorded in the profile. Original game files,
firmware, VM disks and installed runtimes stay outside the repository.
Current private inputs live under `/workspace/school-tycoon-analysis`.

Prepared dependencies:

- QEMU 10.0.13, extracted from signature-verified Debian packages.
- Linux v4.9.170, commit `ad387ec4bcdf087faaf084a3db8f681295463b01`.
  ARM64 defconfig, 39-bit VA, 4 KiB pages, HZ=100, built-in virtio MMIO/block,
  9p virtio, gzip squashfs and virtio RNG. GCC14 needs `HOSTCFLAGS=-fcommon`
  for the older device-tree compiler and `KCFLAGS=-Wno-error`.
- Read-only firmware squashfs and an initramfs with `tools/emulator/init`.
- Separate writable ext4 data disk; disable metadata_csum_seed and orphan_file
  when creating it for kernel 4.9 compatibility. Never modify the raw disk or
  freeze its filesystem while the VM is running.
- ARM64 Wine10/Box64, private assets, software Mesa/Xvfb, and installed PortMaster
  GL4ES/Crusty libraries. Execute runtimes from ext4, not the 9p input share.

## Start and reconnect

Use the extracted QEMU native library path when required by its loader:

```sh
python3 tools/emulator/launch.py \
  --vm-dir /workspace/school-tycoon-analysis/emulator \
  --analysis-dir /workspace/school-tycoon-analysis \
  --qemu /workspace/school-tycoon-analysis/emulator/qemu/usr/bin/qemu-system-aarch64 \
  --socket-console \
  --results-dir /workspace/school-tycoon-analysis/emulator/results
python3 tools/emulator/console.py /workspace/school-tycoon-analysis/emulator/serial.sock
```

The guest has no network device. The input share is `/userdata/host` (read-only);
results are `/userdata/results` (writable). Reconnect to the serial socket rather
than restarting a live test. Check socket ownership before removing any stale
files. Only one serial client should be active.

## Graphics and packaged-game checks

`start-xvfb.sh :95` supplies Wine's private input display and `:96` the fixture's
SDL rendering display; launch each in the background. `graphics-env.sh` selects
software EGL/GLES and the actual GL4ES/Crusty bridge. The software tests use
softpipe to avoid LLVM compilation overhead. SDL's fixture-only display shim
selects the second X display; it is not included in the handheld package.

Compile native-blit, native-shader, native-uniform and native-fpe probes with
the ARM64 toolchain and graphics headers/libraries. They check framebuffer
copy/binding restoration, joined vertex compilation, reflected array bounds and
internal alpha-test variants, including relinks/original source queries.
The FPE baseline fails without the source helper, making it a useful regression.

To test a final archive:

1. Copy it through the input share, then extract into a new guest ext4 directory.
2. Copy `graphics-env.sh` and `run-package.sh` into the prepared private
   `/userdata/host/emulator/shared` fixture location on the host.
3. Ensure the previous test session is stopped. The current fixture starts the
   packaged `st-desktop-ready.exe` and waits for its readiness marker itself.
4. Run `sh run-package.sh /userdata/final-package game-final-package` in the
   guest. It verifies archive file hashes and uses the packaged Box64, native
   graphics helpers, bounded loggers and memory sampler.
5. Capture with `sh capture.sh :96 frame.ppm`; wait for its Captured marker.
   Inspect the resulting image on the output share.

The historical passive keeper works around slow VM explorer startup. The current
handheld package instead has `st-desktop-ready.exe`, which creates a hidden window,
writes readiness, and exits when the original game window exists. Its marker
and retirement are verified in the ARM64 guest. Build the historical keeper
only when reproducing old sessions, compiling `windows-desktop-ready.c` with MinGW, `-Os -nostdlib -mwindows
-Wl,-e,_desktop_start -lkernel32 -luser32`. It creates a hidden window and reads
messages; it does not send input. This is not a proven handheld requirement.

Input requires focus on the native Wine SchoolTycoon window, not the SDL backend.
Find it with native xdotool `search --class .` and inspect window names; title
search alone can miss legacy WM_NAME. Enter can skip the intro when focused.
`click-game.sh X Y` uses native XTest and a PE32 observer which acknowledges
the original engine's cached position/down/up state. Wait for `ST-CLICK-PASS`
and a subsequent rendered screen before the next action; immediate input can
target a previous frame under slow TCG. The observer is fingerprint-specific,
read-only and excluded from the handheld package.

`verify-controller-exit.sh` uses real virtual SDL input and the same exit
decision as the launcher to stop only the owned original game and Wine prefix.
This checks simulated Guide/Fn + Start, not physical button mapping.

## Current evidence

The corrected game build rendered intro/menus/school, placed a Portable English
classroom, reduced cash $8000→$6500, wrote a changed save, and advanced to Day 3
with eight students. Sustained scene memory remained roughly 688–701 MiB RSS
with zero process swap during sampled scenes. Earlier recurring internal FPE
link errors disappeared. CPU interpreter remains the stable baseline.

The final archive's stripped native FPE probe passes in the actual ARM64 guest,
and its exact packaged binaries have rendered the original animated intro.
Fresh reload of the changed NewSave restored the classroom and $6500 budget.
Final native binaries match those used for that scene; the packaged desktop
helper also passed readiness and retirement against the original game.
See [final validation](final-validation.md) for test scope and shutdown results.

Not certified: native Mali/display launch, panel scaling, audio, physical input,
large schools, long-term save compatibility or playable handheld speed. The
chronological evidence and earlier failures are preserved in
[emulator-history.md](emulator-history.md).

### Audio probes used by the second audit

`tools/emulator/windows-audio-probe.c` generates its own DirectSound PCM tone;
it includes no original assets. `windows-music-probe.c` checks Performance8 audio
initialization and optionally loads the user's original SchoolTycoon.spt when
`ST_AUDIO_DIRECTORY` points to its Windows music-directory path. The music
fixture shows its window and pumps messages during the playback observation.
These probes are development tools and are excluded from the handheld archive.

For a PE32 MinGW compiler, build the music fixture with:

```sh
i686-w64-mingw32-gcc -Wall -Wextra -Werror -Os -DINITGUID \
  -fdata-sections -ffunction-sections -nostdlib -mwindows \
  -Wl,--gc-sections -Wl,--strip-all -Wl,-e,_music_start \
  -Wl,--no-insert-timestamp tools/emulator/windows-music-probe.c \
  -lkernel32 -luser32 -lole32 -lgcc -o windows-music-probe.exe
```

A private PulseAudio null sink/monitor replaces the missing physical sound card
for these tests. It does not certify speakers, headphones, Bluetooth, latency or
native Scarab audio. Do not install that fixture server on a handheld. Analyze
completed PCM16 captures with `tools/emulator/analyze-audio.py`; it rejects
silence and tiny resampler dither and reports clipping. API success and a moving
playback cursor do not by themselves establish correctly captured sound.
Original recordings and screenshots remain private. See
[fresh validation](fresh-validation.md) for separate builtin/native media results.

QEMU TCG is not a real-time CPU-speed model. Mixer underruns and timing artifacts
in this VM may differ from the physical device. Capture results remain evidence
about the tested fixture, not guarantees about every audio path on Scarab.

## Handheld-specific failure coverage

Run `python3 tools/emulator/check-handheld-failures.py` in addition to software-rendered gameplay. The profile now includes the reported 64 MiB CMA budget; new boots use `cma=65536K`. See [display/Quick Resume audit](handheld-display-audit.md) for coverage and the limitations of the Mali substitute.

The GPU startup regression additionally supports a real Mesa GLES3 check:

```sh
python3 tests/gpu-warmup-integration.py /path/to/libGLESv2.so.2
```

This verifies shader/draw/readback and native state restoration. It is separate
from the 14 dependency-light regression suites. The package runner preserves
initial Wine output and rejects repeated allocation/framebuffer errors, just as
the handheld runner does. Neither check substitutes for a Mali hardware test.

Set `ST_EXPECT_CONTEXT_HANDOFF=1` when running the native-blit probe to require
real EGL release and rebinding across two threads. This fails against the old
Crusty-only logical binding and passes with the native binding correction.
`python3 tests/context-binding-integration.py` additionally verifies failed-bind
propagation using a controlled native-library fixture.

Set `ST_EXPECT_TEXTURE_READBACK=1` on the same probe to isolate the 4×4
attached-texture update/clear/readback operation implicated by the handheld
Wine adapter-discovery log. It checks both RGBA/unsigned-byte and
BGRA/reversed-packed formats, reports errors at each stage, and compares every
pixel against the expected clear color and native GLES readback. Translated
readback runs first so a diagnostic native read cannot flush the failing
operation in advance. Compile the probe with `-pthread -lX11 -ldl`.

The current packaged helpers pass this probe on ARM64/Mesa (exit 0), including
context handoff, shader linking and framebuffer copy. This does not reproduce
or resolve the reported Mali failure. No new handheld package was issued for
this diagnostic extension; the production runtime is unchanged.


The package runner now mirrors production's `BOX64_MMAP32=0` policy. For an
isolated comparison, build `native-mapping-probe.c` as an ARM64 shared library
(`-shared -fPIC -ldl`) and preload it ahead of the production graphics helpers
while running a PE32 Direct3D probe through Box64/Wine. Compare explicit values
of BOX64_MMAP32=1 and 0 in separate processes. The marker records a native mmap
on the first GLX bind; it is not a GPU-allocation trace. Do not preload it into
normal handheld sessions. See the display audit for the measured addresses.
