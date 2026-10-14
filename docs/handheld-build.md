# 1 GiB handheld package

The October 14 package includes unchanged upstream Box64 revision
45f85efc52d7bd3a3c8642bfc5093ce7932f6c23 built with `-O2 -DNDEBUG`,
replacing the earlier `-O0` build. Its SHA256 is
`fdf6581debf39fcdea490f9a36e7b7578b2687531ed2dc2e910ddeed09f51de8`.
Maximum required glibc version is 2.38; device reports 2.40.

Build the graphics helper with the ARM64 GCC 14 cross compiler and target sysroot:

```sh
aarch64-linux-gnu-gcc -Wall -Wextra -Werror -O2 -fPIC -shared \
  tools/st-gl-present.c tools/st-gl-shader.c tools/st-gl-uniform.c \
  tools/st-gl-source.c -ldl -pthread -o libst-gl-present.so
aarch64-linux-gnu-gcc -Wall -Wextra -Werror -O2 \
  tools/st-controls.c -ldl -o st-controls
```

Replace these binaries and launcher scripts in the public play archive; include
the Box64 license and runtime libgcc license/provenance. Do not include game data,
saves, prefixes, cpu-mode.txt, VM tracing libraries or software Mesa. Runtime-update
now includes the optimized CPU and controller binaries, so it is approximately
6 MiB rather than the earlier script-only update. SHA256SUMS accompanies archives.

Defaults: interpreter CPU, `MALLOC_ARENA_MAX=2`, Wine `csmt=0`, error-only logging.
Wine can schedule independent service work across CPUs; forcing parallel rendering
is unsafe with this bridge's single native context. The optional dynarec remains
experimental. Heap settings bound arena count rather than impose a hard RAM limit.

VM evidence: original intro and scene; classroom placement and save; Day 1;
roughly 687–701 MiB game RSS, no process swap during observed scene tests. This
excludes other system processes and does not predict Mali allocation behavior.
Simulated SDL Fn/Guide + Start and Select + Start exit tests pass; physical Fn
mapping, Mali gameplay, audio and handheld performance remain unverified.

Package validation passed launcher orchestration, prefix preservation/repair,
real Wine installation/cache recovery, native GLX configuration lifetime,
controller integration, ARM64 ELF inspection and archived file checksums.
