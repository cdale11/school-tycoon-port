# Handheld release candidate

The current public archive is `school-tycoon-handheld.tar.gz`, built with
`tools/build-handheld.py`. It excludes original assets, saves/settings, Wine
prefixes, diagnostics, QEMU, Mesa, duplicate update notes and cached downloads.
Licenses, source provenance and libgcc support are required runtime files.

## Reproduce the build

Supply the unchanged pinned Box64 O2 build and an earlier public play archive
for its Box64 license and Debian libgcc license/library. The builder only reads
an explicit allowlist from that archive. Box64 input and libgcc hashes are
verified; other binaries are rebuilt from this repository's sources.

```sh
python3 tools/build-handheld.py \
  --native-directmusic --cab-runtime build-inputs/cab-runtime.tar.gz \
  --box64 /path/to/box64-opt-build/box64 \
  --runtime-base releases/school-tycoon-play.tar.gz \
  --cc /path/to/aarch64-linux-gnu-gcc-14 \
  --sysroot /path/to/arm64-sysroot \
  --strip /path/to/aarch64-linux-gnu-strip \
  --mingw /path/to/i686-w64-mingw32-gcc-posix
```

Use the cross compiler's documented host library path if its loader needs one.
Own binaries use `-O2 -mtune=cortex-a53`; the graphics helper links all four
sources: presentation, joined shader, uniform reflection and source caching.
Runtime ELF symbols are stripped without removing dynamic exports. Symbol
stripping reduces storage and does not itself reduce mapped process memory.
Tar metadata and gzip timestamps are normalized for reproducible archives.
The PE32 readiness helper also uses a normalized link timestamp.
The archive carries a source-content build ID and per-file SHA256SUMS; the
launcher verifies them before running.

Box64 upstream revision `45f85efc52d7bd3a3c8642bfc5093ce7932f6c23` is unchanged,
built with `-O2 -DNDEBUG`, replacing the former `-O0` build. Unstripped input:
`fdf6581debf39fcdea490f9a36e7b7578b2687531ed2dc2e910ddeed09f51de8`.
Stripped package binary: `168b82ee8995f8235d872360966e8a19f253a82330d0917bdbdae55a15f7dbf8`.
Its maximum glibc requirement is **2.39** (including libm), below Scarab's 2.40.
The preceding documentation incorrectly listed 2.38 by overlooking libm.

The current release uses `--native-directmusic --cab-runtime
build-inputs/cab-runtime.tar.gz`. The archive contains the unchanged CAB tools,
licenses/source archives and a checked `runtime/directmusic-enabled` marker.
Microsoft DLLs are downloaded and verified on the device, not redistributed.
The CAB input hash is
`5cf7f9a7f452df470c6e7f9aa917b416a21d4cee37c8a57b18d567c28d26e510`.
Building without either media flag remains a builtin-only developer option;
`--experimental-directmusic` retains the earlier opt-in developer behavior.

The repaired prefix restores `.acm` modules as well as DLLs/EXEs/drivers. Readiness
requires `msadp32.acm` and opens an ADPCM-to-PCM decoder matching the original
music format. The prior COM-only check missed this failure. Existing nonempty
modules, saves and settings are preserved. Native media needs conservative
dynarec; the interpreter fallback uses builtins. `audio-mode.txt` containing
`builtin` also selects the fallback without changing the CPU mode.

## Runtime and memory

Defaults: conservative game/desktop dynarec (interpreted prefix repair), `MALLOC_ARENA_MAX=2`, Wine `csmt=0`, error-only logging.
Wine schedules independent service work across cores. Forcing threaded rendering
is unsafe because the bridge owns one native context. Dynarec remains optional
and unverified; 137 in past runs does not by itself establish an OOM cause.
No firmware governor, overclock, swap policy or game settings are changed.
Arena count is bounded; there is no hard process-memory guarantee.

The corrected FPE shader sources removed recurring link failures and associated
memory growth observed in the earlier VM build. A sustained original scene used
approximately 688–701 MiB RSS, with no process swap; gameplay reached Day 3 and
eight students. These figures exclude other guest processes and are software
Mesa observations, not native Mali memory predictions. Low detail/800x600 are
recommended in-game. Further texture reduction or CPU recompiler defaults are
not justified by current validation.

## Bounded diagnostics

Two small native drainers capture game stdout/stderr. Each retains the current
256 KiB segment plus a preceding segment. Logging storage failure drains rather
than breaking the pipe and killing the game. Output buffers are 4 KiB. Runtime
telemetry reads process/system memory every 30 seconds, retaining 120 samples.
A small PE32 desktop-readiness helper creates a hidden private X11 window before
the game starts. It writes a checked readiness marker and exits when the original
game window appears; it does not stay resident during gameplay. Failure or a
180-second timeout stops startup with desktop.txt, rather than treating headless
cmd.exe success as GUI readiness. Its marker and automatic retirement were
verified with the real ARM64 guest and original game.

No allocator interposition, heap dumps, software renderer or debug tracing is
loaded in normal gameplay. Debug mode adds graphics warnings and Box64 library
startup messages, with bounded output. System/build/library/storage snapshots
and controller connection messages accompany exit status. Full debug logs cannot diagnose every failure.

## Validation limits

`tests/log-integration.py` verifies rotation, exact retained output and drain on
storage failure. Launcher tests verify original-game command, working directory,
graphics selection, CPU mode, failure propagation and owned cleanup. Prefix,
installer and native controls tests cover preservation/recovery and Fn exit.
The virtual SDL fixture does not certify hardware Fn mapping.

`tools/emulator/run-package.sh` loads the exact extracted archive binaries and
logger/telemetry into the prepared ARM64 guest. It verifies package hashes,
substitutes software GLES and two X displays, and keeps its output separate.
The fixture uses the packaged temporary desktop helper. This does not exercise
KNULLI's physical display launch path or remove the need for device validation.
See `docs/emulator.md` for the prepared guest and unsupported hardware.

## Audit hardening

The native `st-lock` helper acquires a process-owned kernel lock before log
rotation or Wine setup. Concurrent launchers are rejected; crashes release the
lock without stale PID cleanup, and forked children do not retain it. Child
shutdown checks PID creation times and bounds TERM/KILL waits. A failed controller
helper stops the session so the documented exit shortcut remains dependable.

Shader source switching uses revision IDs instead of retaining a second source
copy per shader. Pending-deleted shaders are reclaimed after final program
destruction, and reused IDs invalidate stale entries. Package integrity checks
remain active under Python optimization. Release output is written to a temporary
file and replaced atomically only after a successful build.

The second audit adds monotonic startup/shutdown deadlines, native PulseAudio
connection preservation and restoration of shaders beyond the eighth attachment
on fallback linking. A truly new prefix uses wineboot --init (not a forced -u),
with a bounded 600-second initial registration gate. See the fresh-validation
report for the observed first-attempt timeout and corrected-run results.
