# First code audit — 2026-10-14

See the subsequent [fresh validation and second audit](fresh-validation.md) for
the latest package and fresh-install results.

This audit covers the owned launcher, native bridge helpers, controller helper,
logging and release builder. Upstream Box64, Wine, GL4ES and Crusty remain
unchanged. Passing these checks does not certify native handheld gameplay.

## Fixes

| Finding | Change | Regression evidence |
| --- | --- | --- |
| Pending-deleted shaders retained cached source after their final program disappeared; reused IDs could return stale source. | Prune destroyed shaders and invalidate reused IDs. Use source revision IDs instead of retaining a duplicate backend string per shader. | 1000 deletion cycles leave zero entries; shared-program switching preserves original source. |
| Uniform bounds assumed consecutive locations and an active element zero. | Query each reflected array element location and bound writes by the matched element. | Inactive base and nonconsecutive locations, vector/matrix writes, scalar validation. |
| EGL priming terminated a display shared with SDL. EGL initialization is not reference-counted. | Keep the priming reference through process lifetime; do not terminate another owner's display. | Mock native EGL checks destruction without shared-display termination. |
| Shutdown could wait indefinitely for helpers; PID-only signals risked targeting a recycled PID. | Match kernel creation time (including names containing spaces) and bound TERM/KILL waits. Stop gameplay if the controller helper fails. | Stalled helper, failed controller, desktop gate, shortcut and cleanup integration cases. |
| Controller detach could leave injected buttons held. | Release held keyboard/mouse state before reopening input. | SDL virtual-controller tests for Guide, Select and configured L3 exit modifiers. |
| Concurrent launches could rotate live logs and mutate the same prefix/runtime. | Acquire a native process-owned kernel lock before mutable setup. | Concurrent rejection, exec retention, crash release and forked-child ownership tests. |
| Python optimization disabled assertion-based runtime integrity checks; failed output writes could replace a release partially. | Explicit checksum rejection, regular-file/size checks, staged output and atomic replacement. | `python -O` rejects altered input and preserves the previous archive. |

Removed unused diagnostic branches from the production launcher and superseded
binary diagnostic archives from the current tree. Historical downloads use an
immutable commit link. Diagnostic source tools remain available for development.
Original game files and saves remain private.

## Memory and gameplay defaults

The source cache now avoids a second retained source copy per shader and avoids
allocating that copy on program switches. Cache deletion is verified directly;
no measured whole-game RAM reduction is claimed from this change alone.
Cortex-A53-tuned helpers, optimized Box64 interpreter, two allocator arenas,
single-context rendering and bounded logs remain the defaults. Enabling dynarec
or threaded rendering without evidence could reduce stability, so neither is
forced. Previous original-game scene measurements are recorded separately in
[previous package validation](final-validation.md).

## Validation

The eight integration suites for source cache, uniform cache, EGL ownership,
build integrity, session locking, entry handling, logging and launcher lifecycle
passed. Actual ARM64 software-GLES runs of the packaged framebuffer, shader,
uniform and FPE probes passed. These exercise the real native bridge, rather
than shell stand-ins. The original-game audit run passed desktop readiness and rendered its animated
Cat Daddy intro with the audited native binaries. A fresh large-school or
sustained gameplay benchmark was not performed in this audit. Previous gameplay
results remain evidence for the preceding package, not a new performance claim.

## Remaining limits

The bridge targets one native context. It is not a general multi-context OpenGL
implementation. GL4ES itself retains assumptions about consecutive uniform
locations and active array bases; the new bounds cache does not repair all
vendor metadata behavior. Its deferred program deletion also differs from
standard GL semantics. Upstream code was not modified. GLX configuration storage
is retained for process lifetime and repeated enumeration remains a potential
memory concern. Unsupported shader patterns can still fail.

The VM uses a virtual Cortex-A53 board and software GLES with older KNULLI
userspace. Native Mali graphics memory, panel/audio, physical Fn mapping and
usable frame rate still require actual-device evidence. No 1:1 emulator or
100-percent device guarantee is claimed.

## Artifact at commit 1fb96a4

`school-tycoon-handheld.tar.gz`: 5,447,908 bytes. SHA256:
`e95c7e08fa74d0da323b02aee96da7064f6a4bef203c7b5e877261fa58ed3db2`.
All member checksums passed. Final native binaries match byte-for-byte the
archive used for the ARM64 probes. Play/runtime-update aliases contain the same
bytes. Host native EGL initialization and real GLES pixel readback also passed.
The previous EGL implementation reproduces the ownership regression (exit 87);
the fixed helper passes. GLX configuration lifetime regression also passes.
The builder produced byte-identical output in the repeat-build check before
the final PID parsing script correction.

To run the self-contained audit regressions from the repository root:

```sh
for suite in source-cache uniform-cache egl-ownership build-integrity session-lock entry log launcher; do
    python3 "tests/$suite-integration.py" || exit 1
done
```

These require Python 3 and a native C compiler. Real EGL, SDL/X11 and ARM64
checks need the separately documented development runtimes.

The audited ARM64 controller helper passed simulated Guide/Fn + Start against
the owned original-game session, including movement, clicks, Start-alone
preservation and held-button release. It stopped the game with intentional
SIGTERM status 143; production launcher shortcut handling returns zero.
Physical Fn mapping remains unverified.
