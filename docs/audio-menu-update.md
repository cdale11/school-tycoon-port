# Audio, display and controls update — 21 October 2026

The user reports working graphics and improved speed on the RG34XXSP, but no
sound, unwanted side bars, mouse-style menus and slow startup/school loading.
This update addresses three concrete issues and accelerates desktop startup.
School-loading performance and physical audio still need handheld confirmation.

## Audio failure and repair

Wine's native DirectMusic COM/script calls returned success while music stayed
silent. A direct original-segment test exposed `DownloadSegment: 0x80004005`.
ACM tracing showed ADPCM-to-PCM `acmStreamOpen` returning 512 (not possible).
The runtime contained `msadp32.acm`, but the PE32 prefix did not. Prefix repair
copied DLL/EXE/DRV/SYS files and omitted ACM codecs.

Repair now includes `.acm` files and checks for the PE32 ADPCM codec even in an
otherwise verified prefix. The media helper opens a stereo Microsoft ADPCM
44.1 kHz decoder matching the original assets before marking readiness. Missing
codecs trigger repair without deleting the prefix or overwriting nonempty files.
Native DirectMusic is enabled in this release; its pinned Microsoft files are
extracted on-device from a verified download, never bundled or uploaded here.

Software ARM64 VM results with conservative dynarec and precise floating-point
settings:

- Native DirectSound tone: 225,362 nonzero samples, zero clipped samples.
- Original music segment after codec repair: load/download/play succeeded;
  1,130,760 nonzero samples and zero clipped samples in the bounded recording.
- Original game: intro and full-panel Main/Settings screens rendered. A separate
  44.8-second menu recording contained 3,100,476 nonzero samples, zero clipped
  samples, and peak magnitude 17,353 in signed 16-bit PCM.
- The full startup recording contained 5,976 clipped samples before the clean
  menu recording. Startup audio quality and A/V synchronization are not certified.
- The packaged prefix helper passed both PE32 execution and media/ADPCM checks.

Test volumes were raised in a private fixture. The release does not overwrite
user volume settings: raise Sound Effects and Music in Settings if needed.
Native audio distorted earlier interpreter tests, so `cpu-mode.txt` containing
`interpreter` selects builtin media. Remove that file for the default dynarec
path. `audio-mode.txt` containing `builtin` is a separate audio fallback.

## Display and controls

The original 800×600 full-frame copy now stretches to all 720×480 output pixels.
Cursor coordinates use the same transform. Partial/offscreen copies and GL
state remain intact; no per-frame bar-clearing operation is needed. Keep the
game at 800×600. Intrinsic letterboxing in an intro asset is not a side-bar bug.

The verified executable gets D-pad selection on Main, Play Select and Credits;
A activates the hovered button. The first direction focuses the first button.
Settings, school lists and gameplay retain pointer control. L1 always enables
precise pointer control. This is an input adapter, not native gamepad support
inside the original engine.

The adapter reads the owned process only after the launcher verifies its exact
executable hash. It validates the manager, active-screen list and known screen
type; unknown/modal screens, unavailable process access, unsupported executables
or settings other than 800×600 fall back to the pointer. It never writes game
memory. SDL virtual-controller testing moved from Play to Settings, clicked A,
and verified that the actual game entered Settings. Held-input release and
Guide/Select+Start remain covered; physical Fn mapping is not certified by that
fixture. `tools/emulator/menu-controls-test.py` reproduces the private game test.

## Loading, memory and limits

Desktop startup now uses the game's conservative dynarec configuration; prefix
creation/repair retains its interpreter path. Phase durations are logged to
separate prefix work from desktop delay. This does not remove the intro or prove
that a school loads in less than 30 seconds. No school-loading speedup is claimed.

The native-audio menu fixture used approximately 345 MiB game RSS with zero game
swap in its final samples; total VM available memory was about 144 MiB. This is
not a new-school memory measurement or a Mali memory prediction. No overclock,
firmware governor, swap policy or forced multithreaded rendering is introduced.

The VM substitutes software graphics and a virtual audio sink. It cannot certify
Mali presentation, physical speakers, button mappings or real-time performance.
The package remains a release candidate. All game files and recordings remain
private and are excluded from GitHub and the downloadable package.

## Final launcher check — 27 October 2026

The launcher now reloads media selection after prefix repair. Installation runs
in a child shell; without the reload, the first launch retained builtin overrides
until a second launch. A regression test creates native readiness during repair
and verifies native overrides in that same game launch. It passed along with
prefix preservation/reuse, menu bounds/resolution checks, full-panel presentation,
firmware-setting audit and package-integrity checks. The final archive is rebuilt
from this correction; the VM capture results above remain dated 21 October.
