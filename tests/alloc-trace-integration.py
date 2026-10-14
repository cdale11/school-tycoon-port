#!/usr/bin/env python3
"""Verify native allocation instrumentation against real glibc allocations."""
import pathlib
import os
import re
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
fixture = r'''
#include <stdlib.h>
#include <string.h>
int main(void) {
    void *p = malloc(262144); if (!p) return 1; memset(p, 1, 262144);
    p = realloc(p, 524288); if (!p) return 2; free(p);
    p = calloc(2, 131072); if (!p) return 3; free(p);
    if (posix_memalign(&p, 64, 262144)) return 4; free(p);
    p = aligned_alloc(64, 262144); if (!p) return 5; free(p);
    for (int i = 0; i < 100; i++) {
        p = malloc(131072); if (!p) return 6; memset(p, 2, 131072); free(p);
    }
    return 0;
}
'''
exports = r'''
#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
extern void *__libc_malloc(size_t), *__libc_calloc(size_t, size_t);
extern void *__libc_realloc(void *, size_t), *__libc_memalign(size_t, size_t);
extern void __libc_free(void *);
void *malloc(size_t n) { return __libc_malloc(n); }
void *calloc(size_t n, size_t s) { return __libc_calloc(n, s); }
void *realloc(void *p, size_t n) { return __libc_realloc(p, n); }
void free(void *p) { __libc_free(p); }
void *memalign(size_t a, size_t n) { return __libc_memalign(a, n); }
void *aligned_alloc(size_t a, size_t n) { return __libc_memalign(a, n); }
int posix_memalign(void **p, size_t a, size_t n) {
    if (a < sizeof(void *) || (a & (a - 1))) return EINVAL;
    void *q = __libc_memalign(a, n); if (!q) return ENOMEM; *p = q; return 0;
}
int main(int argc, char **argv) {
    if (argc != 2) return 10;
    void *h = dlopen(argv[1], RTLD_NOW); if (!h) return 11;
    int (*run)(void) = dlsym(h, "run_fixture"); if (!run) return 12;
    int status = run(); dlclose(h); return status;
}
'''

def verify(result):
    final = result.stderr.rsplit("ST allocations ", 1)[1]
    assert "events=105 dropped=0" in final, final
    rows = re.findall(r"live=(\d+) peak=(\d+) total=(\d+) count=(\d+)", final)
    assert len(rows) == 6 and all(int(row[0]) == 0 for row in rows), final
    assert sum(int(row[2]) for row in rows) == 14680064, final
    assert max(int(row[1]) for row in rows) == 524288, final

with tempfile.TemporaryDirectory(prefix="st-alloc-test-") as tmp:
    tmp = pathlib.Path(tmp)
    source, binary, helper = tmp / "fixture.c", tmp / "fixture", tmp / "trace.so"
    source.write_text(fixture)
    subprocess.run(["cc", "-O0", str(source), "-o", str(binary)], check=True)
    subprocess.run(["cc", "-Wall", "-Wextra", "-Werror", "-O2", "-shared", "-fPIC",
                    str(root / "tools/emulator/alloc-trace.c"), "-ldl", "-lpthread",
                    "-o", str(helper)], check=True)
    result = subprocess.run([str(binary)], env={**os.environ, "LD_PRELOAD": str(helper)},
                            text=True, capture_output=True, check=True, timeout=15)
    verify(result)
    source.write_text(exports)
    subprocess.run(["cc", "-O0", "-Wl,--export-dynamic", str(source), "-ldl",
                    "-o", str(binary)], check=True)
    source.write_text(fixture.replace("int main(void)", "int run_fixture(void)"))
    library = tmp / "fixture.so"
    subprocess.run(["cc", "-O0", "-fPIC", "-shared", str(source), "-o", str(library)], check=True)
    subprocess.run(["cc", "-Wall", "-Wextra", "-Werror", "-O2", "-shared", "-fPIC",
                    str(root / "tools/emulator/alloc-audit.c"), "-ldl", "-lpthread",
                    "-o", str(helper)], check=True)
    verify(subprocess.run([str(binary), str(library)],
                         env={**os.environ, "LD_AUDIT": str(helper)},
                         text=True, capture_output=True, check=True, timeout=15))
    filtered = subprocess.run([str(binary), str(library)],
                              env={**os.environ, "LD_AUDIT": str(helper),
                                   "ST_ALLOCATION_MIN_SIZE": "67108864"},
                              text=True, capture_output=True, check=True, timeout=15)
    assert "events=0 dropped=0 minimum=67108864" in filtered.stderr, filtered.stderr
print("PASS: preload and executable-allocator audit; aligned allocation, release, no dropped records")
