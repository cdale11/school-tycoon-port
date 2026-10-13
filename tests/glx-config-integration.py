"""Check configuration lifetime after callers free the enumeration array.
Uses a small native backend with the bundled Crusty allocation layout.
This validates the helper contract, not game rendering or Mali hardware.
"""
from pathlib import Path
import os
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="st-glx-config-") as directory:
    root = Path(directory)
    backend = root / "backend.c"
    backend.write_text(r'''#include <stdlib.h>
struct config { int value; };
void **crusty_glXGetFBConfigs(void *display, int screen, int *count) {
    (void)display; (void)screen;
    void **array = malloc(sizeof(void *) + sizeof(struct config));
    struct config *config = (void *)(array + 1);
    config->value = 12345; array[0] = config; *count = 1;
    return array;
}
void **crusty_glXChooseFBConfig(void *display, int screen, const int *attrs, int *count) {
    (void)attrs; return crusty_glXGetFBConfigs(display, screen, count);
}
''')
    driver = root / "driver.c"
    driver.write_text(r'''#include <assert.h>
#include <stdlib.h>
#include <string.h>
struct config { int value; };
extern void **crusty_glXGetFBConfigs(void *, int, int *);
extern void **crusty_glXChooseFBConfig(void *, int, const int *, int *);
int main(void) {
    for (int mode = 0; mode < 2; mode++) {
        int count = 0;
        void **array = mode ? crusty_glXChooseFBConfig(0,0,0,&count) : crusty_glXGetFBConfigs(0,0,&count);
        assert(count == 1); struct config *config = array[0];
        assert(config->value == 12345); free(array);
        void *churn[128];
        for (int i=0;i<128;i++) { churn[i]=malloc(sizeof(void *)+sizeof(struct config)); memset(churn[i],0,sizeof(void *)+sizeof(struct config)); }
        assert(config->value == 12345);
        for (int i=0;i<128;i++) free(churn[i]);
    }
    return 0;
}
''')
    def run(*args):
        subprocess.run(args, check=True, capture_output=True, text=True)
    run("cc", "-shared", "-fPIC", str(backend), "-o", str(root / "backend.so"))
    run("cc", "-shared", "-fPIC", "-Wall", "-Wextra", str(repo / "tools/st-glx-config.c"), "-ldl", "-pthread", "-o", str(root / "helper.so"))
    run("cc", str(driver), str(root / "backend.so"), "-o", str(root / "driver"))
    env = dict(os.environ, LD_PRELOAD=str(root / "helper.so"), ST_CRUSTY_LIBRARY=str(root / "backend.so"))
    result = subprocess.run([str(root / "driver")], env=env, capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, (result.returncode, result.stderr)
    assert "retained GLX framebuffer configurations" in result.stderr, result.stderr
    baseline = subprocess.run([str(root / "driver")], env=dict(env, LD_PRELOAD=""), capture_output=True, timeout=10)
    assert baseline.returncode != 0, "Fixture did not reproduce the original lifetime failure"
    print("PASS: caller-owned arrays, stable configurations, both enumeration functions; baseline reproduces failure")
