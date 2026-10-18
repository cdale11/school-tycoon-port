# Cloud workspace setup and checks

The repository's fast regressions need Python 3, a Linux C compiler (`cc`),
POSIX shell/core utilities and development headers for libc, pthreads and dlopen.
They generate their own fixtures; no original game files, Wine installation,
network connection, root access or running handheld is required.

From the repository root:

```sh
for suite in source-cache uniform-cache egl-ownership glx-config build-integrity \
             session-lock entry log launcher prefix-repair audio-analysis \
             native-startup firmware-settings installer-signal presentation-fit; do
    python3 "tests/$suite-integration.py" || exit
done
```

These checks passed in this cloud workspace. The controller integration needs
SDL2, X11/XTest and a real or virtual X display; see docs/emulator.md for its
prepared ARM64 guest execution. Passing synthetic launcher tests does not
establish that the proprietary game runs.

The optional Wine installer test exercises the actual pinned Wine 10.0 archive
without a network download:

```sh
python3 tests/wine-install-integration.py /private/path/wine-10.0-amd64-wow64.tar.xz
```

For builds, supply the pinned unchanged Box64 input, an ARM64 cross compiler and
sysroot, strip and a PE32 MinGW compiler as described in handheld-build.md.
Compiler toolchains can be staged outside the repository in /workspace; they
are not included in the handheld archive. The default build does not need the
experimental CAB input or Microsoft redistributable. Repeated builds normalize
archive metadata and PE timestamps; compare the resulting hashes.

Full original-game testing requires privately supplied assets and the prepared
QEMU environment. Keep assets, prefixes, game screenshots and recordings outside
the repository. Current audit observations and failures are in fresh-validation.md;
the VM is not an exact Scarab/H700/Mali emulator or a handheld certification.

Native context binding regression (host compiler only):

```sh
python3 tests/context-binding-integration.py
```

This checks native release, transfer between threads, both GLX binding APIs and
propagation of native binding errors. The ARM64 native-blit probe additionally
checks real EGL behavior with `ST_EXPECT_CONTEXT_HANDOFF=1`.
