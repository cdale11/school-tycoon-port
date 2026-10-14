# Previous package validation — 2026-10-14

This records the package published at commit `945d7b0`, before the subsequent
[code audit](code-audit.md). Its artifact hashes and gameplay measurements apply
to that previous build. This is a release candidate. The tests below do not certify a playable native
Mali port. QEMU cannot completely emulate RG34XXSP hardware or Scarab's GPU,
audio, panel and physical inputs.

## Artifact

`school-tycoon-handheld.tar.gz`: 5,444,609 bytes (about 5.2 MiB).
SHA256: `af3ded0f58c1aac60bcf5d17042be1ca2d57f151f3d2363b0ab136702930504a`.
Source build ID: `74b61003191919d26e11164110f136d3c04575fca6a07a376969bc5854196034`.

22 files: launcher/install/prefix scripts, telemetry, optimized stripped Box64,
three native graphics libraries, controller/log helpers, a small owned PE32
desktop-readiness helper, libgcc, licenses, provenance, manifest and instructions.
No original executable/assets, saves, settings, Wine prefix, Mesa, QEMU,
diagnostic probes or duplicate update notes. Legacy play/runtime-update links
now carry the same bytes as the canonical handheld archive.

Native/Windows runtime binaries and scripts were compared byte-for-byte against
the complete archive extracted into the guest. Final differences from that test
archive only update installation prose and its manifest. The builder produced
identical archives on repeated builds before that prose-only update.

## Completed checks

| Check | Result and scope |
| --- | --- |
| ARM64 compatibility | Six native binaries are AArch64; Box64 maximum glibc requirement is 2.39, below reported 2.40. The readiness program is owned PE32 code for Wine. |
| Archive integrity | All per-file hashes passed on host and actual ARM64 guest; safe paths and no proprietary assets/prefix content verified. |
| Original-game rendering | Intro, main/play/load menus, saved school, trees and classroom rendered through the actual GL4ES/Crusty bridge in ARM64 QEMU with software GLES. |
| Changed-save reload | NewSave restored the placed Portable English classroom and $6500 budget. Earlier placement changed the save from 203182 to 204699 bytes; SHA256 d5308f91414dd9da979b85bb21fb6656a6f035b759b5eee0a29a6fe5dfd279a2. |
| Simulation | Earlier sustained corrected-build session reached Day 3 and eight students. Not every game feature or large-school workload was tested. |
| Memory | Final reload run: 54 samples at 30-second intervals, peak 676.05 MiB game RSS, zero process swap, minimum guest MemAvailable 125536 KiB. Settled samples were about 662 MiB. Earlier longer scene/preview/student runs were approximately 688–701 MiB. |
| GUI readiness | Packaged helper wrote ST-DESKTOP-READY and exited zero with ST-DESKTOP-RETIRED against the real original-game window. From a stopped prefix, it became ready in 118 seconds, within the 180-second gate. |
| Complete cold startup | Current run-package fixture verified all archive hashes, used its temporary desktop helper, and rendered the original animated intro. The helper retired automatically. |
| Simulated exit | Guide/Fn + Start stopped the owned original game in both reload and complete cold-start sessions. ST-GAME-EXIT-PASS recorded. Native game wrapper status 143 is the intentional SIGTERM outcome; production-launcher integration returns zero on the shortcut. |
| Controller regression | Real SDL virtual-controller/X11 tests passed Guide+Start, Select+Start, configured L3+Start, Start-alone preservation and held-button release. HOTKEY=start is rejected as an exit modifier. |
| Shader regression | Stripped packaged helper passed the real ARM64 FPE alpha-discard/joined-source/original-source/array-write fixture, exit zero. Earlier shader, uniform and framebuffer fixtures are recorded in emulator history. |
| Logging | Real native drainer passed bounded rotation, exact retained tail and storage-failure draining. Entry tests preserved bounded preceding logs and saves, reported debug mode and rejected damaged packages. |
| Launcher lifecycle | Original command/cwd/GLES/CPU selection, failure status, desktop readiness/failure gate, controller shortcut, owned process shutdown and logger FIFO cleanup passed with stand-in runtimes and the real native logger. |
| Wine/prefix recovery | Real pinned Wine installer/cache-recovery checks and prefix missing-module restoration/save-preservation tests passed. |
| Native bridge checks | GLX configuration lifetime test passed; host EGL early initialization, real pixel readback, target filtering and explicit native loader tests passed. |

Game RSS is not total guest memory, and GPU allocation behavior differs on Mali.
Some other guest pages used swap despite zero game-process swap. VM logs still
contain Wine service timeouts and OpenGL compatibility warnings; they did not
prevent these observations, and are not all resolved.

## Limits that remain

The virtual board uses an older KNULLI userspace with matching Buildroot/glibc,
not an exact Scarab image. Virtual CPU execution and software rasterization
cannot certify handheld frame rate, native GPU memory, output scaling or sound.
Physical controller/USB input is unverified. Earlier actual-device graphics
startup failed; no current Mali gameplay pass is claimed. Experimental dynarec
is not the default, and forced multicore rendering is unsafe with this bridge.

The package is hardened and ready for a device trial; it is not a guarantee
that the full game will run correctly on the handheld. Use the documented
logs to distinguish installer, prefix, desktop, graphics, controls and memory
failures. No reset or deletion of original data is required.
