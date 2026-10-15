# Second audit and fresh emulator validation — 2026-10-14/15

This report distinguishes code regressions, fresh installation, original-game
behavior and physical hardware. No build is claimed to be 100 percent bug-free,
and this VM cannot certify native RG34XXSP graphics/audio or performance.

## Second-pass changes

| Finding | Fix and evidence |
| --- | --- |
| Fallback linking restored only eight shader attachments. | Restore all attachments before fallback; the ninth-attachment regression fails with the preceding helper and passes after the change. |
| Startup/shutdown deadlines used wall time. | Read monotonic uptime; a frozen-wall-clock regression reproduces the preceding shutdown hang and passes after the change. |
| Private Wayland runtime could hide an existing per-user PulseAudio server. | Preserve its socket before replacing XDG_RUNTIME_DIR; integration tests cover automatic preservation and explicit PULSE_SERVER precedence. |
| Desktop readiness was published before retirement timer setup. | Acquire the timer before publishing readiness. |
| Initial Wine setup timed out in the clean ARM64 VM. | First attempt failed the 300-second gate before game launch. Fresh bootstrap uses --init instead of forcing a subsequent -u and allows a bounded 600 seconds; partial test state was deleted for the next run. |
| VM helpers assumed old game/prefix/runtime/font paths. | Fresh mode targets the extracted package and new prefix; click/exit helpers accept ST_VM_PACKAGE. Font configuration references that runtime. |

A defensive shader-function pointer check was added during GCC analysis.
The warning arose from repeated string matching; it was not a reproduced crash.
GCC analyzer compilation passes for all eight owned native helper translation
units. Source/uniform cache regressions pass under AddressSanitizer and
UndefinedBehaviorSanitizer. Upstream vendor source remains unchanged.

## Fresh-test provenance

Prior guest gamedata and the working prefix were archived to a private host
backup, read fully to verify the archive, and removed from the active guest.
Old diagnostic prefixes and Wine were removed. Untouched uploaded ST assets were
copied to a new package directory; original SAVED_GAMES is empty. Wine was
freshly extracted by the packaged installer from the checksum-pinned archive.
The offline VM reused only that verified download, not installed Wine or an old
prefix. Native graphics libraries and virtual X displays are emulator
infrastructure. Software GLES substitutes for Mali; older KNULLI userspace
substitutes for exact Scarab firmware.

All four native packaged framebuffer/shader/uniform/FPE probes pass in ARM64.
The first fresh prefix attempt failed its 300-second gate and is retained as a
failure. The revised attempt started without a prefix or verification marker.
Its completed results follow below.

## Audio method

The VM has no physical sound card. A private PulseAudio null sink and monitor
capture provide a real native audio server for Wine. A generated calibration
tone was captured with peak 3999 and 13000 nonzero samples at stereo 22050 Hz.
This validates capture, not game audio. Game recordings stay private and are
not shipped. Nonzero PCM proves output, not every sound, synchronization or
physical speaker behavior.

## Rumble

No rumble/force-feedback support was found in the original executable, its two
bundled DLLs or supplied readme. None imports DirectInput or XInput. None of the
21 checked standard DirectInput interface/class/effect GUIDs occurs in the main
executable. No obvious force-feedback, vibration or rumble strings were found.
The readme mentions joystick calibration as a scrolling workaround; that does
not establish haptics. These are binary/document observations, not source-code
proof against a hidden path. The port injects keyboard/mouse input and does not
implement haptics.

The revised genuinely new prefix completed initial Wine registration within
the 600-second gate. Its subsequent command and game checks are recorded
separately; registry creation alone is not a gameplay pass.

The new prefix then passed the actual PE32 command gate with
`ST-PREFIX32-PASS`, and the packaged verifier created its ready marker and
removed the repair-pending marker. This used the real ARM64 Box64/Wine runtime,
not a stand-in shell command.

The packaged temporary desktop helper wrote ST-DESKTOP-READY and retired
automatically after the original School Tycoon window appeared. The original
executable then started with early native EGL initialization. Rendering and
audio observations are assessed separately below. All ten integration suites
(source cache, uniform cache, EGL ownership, build integrity, session lock,
entry, logger, launcher, prefix recovery and GLX lifetime) passed.

## Additional audio failure found by the clean test

The game's PE32 DirectMusic loader failed with CLSID
`{D2AC2892-B39B-11D1-8704-00600893B1BD}` not registered. The corresponding
64-bit class existed, but the 32-bit registration did not. A 178.78-second
monitor capture contained only silence (zero nonzero samples). The game was
stopped; this is an audio failure, not a successful sound check.

The new owned PE32 media helper registers the pinned Wine media DLLs in the
correct architecture and instantiates thirteen media COM classes. Prefix
readiness now requires its exact success marker as well as the CPU gate.
Failed status or a missing marker prevents gameplay. Its verified marker is
keyed to the helper hash, and missing core media DLLs trigger recovery.
The helper exits before gameplay and stays out of the steady-state memory
footprint. Subsequent real runtime results are recorded separately.

The separate actual PE32 DirectSound probe also failed with MMDeviceEnumerator
CLSID `{BCDE0395-E52F-467C-8E3D-C4579291692E}` unregistered. That DLL/class is
now included in the targeted media readiness check. This explains why testing
command execution and graphics alone did not establish sound readiness.

After targeted registration, all twelve actual PE32 COM class checks passed.
The separate DirectSound fixture passed playback/cursor checks. Its native
monitor capture contained 72906 nonzero samples, peak 3470 and zero clipped
samples at stereo 22050 Hz. This proves the Wine-to-native-audio path with the
owned tone fixture; it is not yet a claim about original-game music.

The small media helper is approximately 6 KiB after discarding unused GUID sections and
link symbols. It is not resident during gameplay. Original assets contain PCM
and Microsoft ADPCM waveform data, including embedded WAVE chunks in music
segments; no proprietary system GM/DLS bank is added to the package.

A serial-observed clock check measured 10.08 seconds of guest uptime over
9.92 seconds on the host (including serial latency). No large timer-rate
distortion was observed. CPU execution and software rendering still differ
from the handheld.

The emulator font-config producer also exposed preload diagnostics contaminating
its generated XML. It now clears preloads before generating configuration; the
active fixture file was corrected before CPU/media and game readiness. This
was a test-infrastructure defect, and is included in the emulator cleanup.

## Scripted media compatibility and current native trial

Wine 10.0 DirectMusic script CallRoutine and variable methods are stubs; the
original SchoolTycoon.spt uses VBScript. Wine wave-track ordinary Play has
implementation, while other paths have stubs. Neither successful COM construction
nor a generated DirectSound tone proves original-game audio.

The original uploaded PC settings muted music and effects. Initial silent
captures cannot attribute silence solely to Wine. After raising both volumes,
a menu/effect capture was also silent. The fresh-install guide now explicitly
omits the old settings.dat when a default-settings installation is wanted.

The retired official DirectX URL returned 404. The Winetricks mirror downloaded
the February 2010 redistributable successfully and matched SHA256
`f6d191e89a963d7cca34f169d30f49eab99c1ed3bb92da73ec43617caaa1e93f`.
Nine allowlisted native PE32 DirectMusic DLLs are under test, retaining Wine
DirectSound. No Microsoft DLLs or original game assets are published.

The actual ARM64 extractor installed all nine DLLs with matching individual
hashes, and --check passed. Host integration independently verifies real CAB
extraction, extraction-failure preservation, prefix symlink-target/save
preservation, backups and reuse without a download or extractor. The native
media set passed thirteen PE32 COM class checks including VBScript.

The native-media original game rendered the main menu, Play menu, Instant Action
chooser and new-school loading screen. A 66.764-second menu capture was entirely
silent (peak 0, RMS 0). Original-game sound is **not verified**. Classroom loading,
in-game effects/music and final-package clean installation remain in progress.
Do not interpret this candidate as a production-ready or guaranteed handheld
release.

The preceding Wine-only fresh run passed simulated Fn/Guide + Start through
the real SDL control helper and original game process. Intentional TERM was
recorded as game status 143; the launcher integration treats user exit normally.
Physical Fn mapping is unverified. Previous scene/save/Day-3 results belong to
the earlier build and are not substituted for fresh native-media gameplay.
