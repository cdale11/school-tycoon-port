# Second audit and fresh VM validation — 2026-10-15

**A production-ready full port is not established.** The native-media candidate
passes installation, PE32 execution, COM and desktop gates, but its full-game
audio test fails and its observed display remains black. The release archives
have not been promoted to that experimental build. No version is claimed to be
100 percent bug-free or certain to work on physical RG34XXSP hardware.

## Audit fixes

| Finding | Change and verification |
| --- | --- |
| Fallback shader linking restored only eight attachments. | Restore every attachment before fallback; the ninth-attachment regression reproduces the old failure and passes after the fix. |
| Startup and shutdown used wall-clock deadlines. | Use monotonic uptime; the frozen-clock shutdown regression now passes. |
| Private display setup could hide the existing PulseAudio socket. | Preserve the socket before replacing XDG_RUNTIME_DIR; test automatic preservation and explicit override precedence. |
| Desktop readiness preceded retirement-timer creation. | Create the timer before publishing readiness. |
| A genuinely fresh Wine prefix exceeded the old 300-second gate. | Use wineboot --init with a bounded 600 seconds. Delete partial test state before the new run. |
| PE32 media classes were absent although 64-bit registration existed. | Register and instantiate 13 PE32 classes, including MMDeviceEnumerator and VBScript. Require the helper's status and exact marker; invalidate verification when the helper changes or required modules disappear. |
| The media-ready marker alone did not select a valid media stack. | Verify the pinned DLL set and repair missing marker state. COM readiness is explicitly distinguished from playback. |
| Wine error-only flags retained default fixme channels. | Explicitly disable all channels before enabling errors; launcher regression checks the effective flags. |
| VM tools retained old game/runtime/font paths. | Fresh and resume modes select the extracted package. Clear preloads before producing font XML; click helpers use package-local font configuration. |

The source-cache test covers 1000 pending-deletion cycles, ID reuse and shared
shader switching. Source/uniform tests pass with AddressSanitizer and UBSan.
GCC analyzer compilation passes for eight owned native helper translation units.
Launcher, prefix recovery, Wine recovery, session lock, entry, log rotation,
GLX lifetime, EGL ownership, build-integrity and audio-analysis tests pass.
The defensive shader-function pointer guard addresses an analyzer warning;
that warning was not a reproduced crash. Vendor Box64/GL4ES source is unchanged.

## Fresh-test provenance

The preceding active game and prefix were backed up privately. Active old
prefixes, installed Wine and game files were removed; original uploaded assets
were recopied from the read-only source. The final clean test omits settings.dat
and checks SAVED_GAMES empty. Earlier settings screens showed zero effects/music sliders, but the same
observation recurs after generating default settings. Silence is therefore not
attributed solely to an imported PC profile.

Only checksum-verified download archives were reused. Wine was freshly extracted
by the packaged installer. The 4 GiB virtual data disk first failed the package's
2 GiB free-space gate; it was enlarged to 6 GiB and ext4 grown online. The storage
gate was retained. RAM and CPU configuration did not change.

The ten-DLL candidate archive has SHA256
`d57c219f133e4795e112769112d8070a36efcda272673b01b9909779284290d5`.
A second build is byte-for-byte identical. The clean run passed wineboot, the
actual PE32 command gate, all ten DLL hashes, all 13 media class checks and the
private desktop gate. It then launched the original executable, selected Low
settings automatically and reached DirectMusic search-path setup.

The VM has four Cortex-A53 cores, approximately 1 GiB RAM, 4 KiB pages, 39-bit
address space, kernel 4.9.170 and glibc 2.40. It uses an older KNULLI userspace,
a virtual board, software GLES and virtual displays. It cannot reproduce the
H700/Mali driver, panel, physical sound hardware, button wiring or execution
speed. A clock comparison observed 10.08 seconds of guest uptime over 9.92
host seconds; that does not make TCG a real-time CPU or audio model.

## Observed game and graphics results

The earlier fresh Wine-only run rendered the intro, main menu, settings and
Instant Action chooser. A separate nine-native-DLL trial (retaining builtin
DirectSound) rendered the new Farmland scene, paused at Day 0 with no students,
at approximately 609 MiB game RSS and zero process swap. It passed simulated
Guide/Fn + Start exit through the real SDL helper and game process; intentional
termination was status 143. Physical Fn mapping remains unverified.

Those are distinct from the ten-DLL clean test. Earlier classroom/save/Day-3
results belong to the preceding build and are not substituted for this run.
The framebuffer, shader, uniform and FPE probes pass in ARM64. The Box64 and
three graphics libraries in the ten-DLL candidate are byte-identical to the
binaries used for those probes.

## Audio evidence

The VM uses a private PulseAudio null sink/monitor because it has no physical
sound card. Do not install that server fixture on a handheld. Captures remain
private. The analyzer rejects silence and tiny resampler dither and optionally
rejects a specified fraction of clipped samples.

| Stack / test | Observed result |
| --- | --- |
| Native capture calibration | Generated tone captured; peak 3999. |
| Builtin Wine media, after PE32 registration repair | Owned DirectSound tone API/cursor check passes; capture has 72906 nonzero samples, peak 3470 and no clipped samples. This is not original-game music proof. |
| Original game, initial and post-unmute trials | Silent captures. Settings screens showed zero sliders; the later default-settings screen does too. Volume and initialization are checked separately. |
| Nine native media DLLs with builtin DirectSound | All 13 classes construct, but the owned Performance8::InitAudio probe fails E_NOINTERFACE. |
| Mixed builtin performance/core/synth with native scripting | InitAudio succeeds, but native script Init fails E_NOINTERFACE. One mixed-probe teardown also hits a PulseAudio assertion. |
| Ten native DLLs, including native DirectSound | Owned InitAudio, original script loading, script Init, volume assignment and menu CallRoutine succeed. The old hidden-window fixture's 201.823-second capture is silent. API success is not playback proof. |
| Native DirectSound tone, old default-primary fixture | API/cursor pass; 129962 nonzero samples and 41584 negative-full-scale samples. Quality fails. The revised fixture requests a PCM16 primary buffer but its new output has not replaced this observation. |
| Ten-DLL full game, isolated capture before debugger intervention | 565.000 seconds; 3440 nonzero samples, 3256 full-scale (94.65% of nonzero samples). Almost entirely silence plus an artifact; fails the clipping gate. |

The full-game native stack opens a native Pulse stream and has 11 threads, but
Wine also reports a 60-second wait on WINMM_Device.lock. This establishes a
lock-wait failure, not proof of a permanent deadlock or a hardware result. The
observed display remained black. Debugger attempts could not obtain a valid
backtrace under this Box64/WoW64 stack. The game later exited status 84 during
that intervention; it is not reported as an unassisted crash before debugging.

Wine 10 script CallRoutine/variable methods have stubs. Its ordinary wave-track
Play has implementation while other paths have stubs. The builtin-media comparison with identical files/settings/graphics renders the
animated intro, Cat Daddy logo, main menu and settings screen. This isolates the
native-media black-screen regression. The default build now forces builtin media
and excludes the experimental installer/CAB tools. Media-mode changes invalidate
readiness; tests cover opt-in, opt-out and missing experimental tools.
Its 804.039-second recording contained only 336 noise-level samples (peak 1),
including the interval after raising both sliders. It passed simulated Fn/Guide
+ Start exit again. Original-game music, effects and synchronization are not
certified.

A fresh default-package installation is now running from archive SHA256
`f5086dfc270d8cc8589581ccb61189f38b70502c7e878f6da4e14a8d4f012a09`.
The native-containing prefix, installed Wine and game files were removed again;
only the verified Wine download was reused. This archive excludes the native
installer, CAB runtime and source bundle. It passed wineboot, actual PE32 execution, all 13 builtin media classes and
desktop readiness without the native experiment, then launched the original
executable. Its media marker ends in :builtin. These results are separate from
the previous native-prefix comparison. The test sink is unmuted at 100%. The clean default run renders the original animated intro, Cat Daddy splash
and main menu with no native-media files installed; a separate capture remains
noise-level during that observation. A new-school load is checked separately.

## Runtime provenance and preservation

The retired official DirectX URL returns 404. The Winetricks mirror supplied the
February 2010 redistributable with SHA256
`f6d191e89a963d7cca34f169d30f49eab99c1ed3bb92da73ec43617caaa1e93f`.
The candidate extracts ten allowlisted PE32 DLLs and verifies each hash. No
Microsoft DLLs or original assets are published. The ARM64 CAB tools are
unchanged Debian binaries with licenses and complete corresponding sources.
Real extraction tests cover failure preservation, directory rejection,
symlink-target/save preservation, backups and reuse without another download.
Native DirectSound lacks DllRegisterServer; the helper provides only missing
standard PE32 DirectSound COM mappings in that case.

## Original rumble support

No rumble/force-feedback support was found in the executable, its two bundled
DLLs or supplied readme. None imports DirectInput/XInput; none of 21 checked
standard DirectInput class/interface/effect GUIDs occurs in the main executable.
No obvious rumble/vibration/force-feedback strings were found. The readme's
joystick-calibration workaround concerns scrolling, not haptics. These binary
observations are not source-code proof against a hidden path. The port injects
mouse/keyboard input and does not implement haptics.
