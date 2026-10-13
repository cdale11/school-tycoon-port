/* SPDX-License-Identifier: MIT
 * Reproduce Crusty's GLXFBConfig lifetime failure with its real native library.
 * Run in the ARM64 VM with graphics-env.sh. Status 4 is a lifetime failure.
 */
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    void *library = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);
    if (!library) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    Display *display = XOpenDisplay(NULL);
    if (!display) return 2;
    void **(*get_configs)(Display *, int, int *) = dlsym(library, "glXGetFBConfigs");
    int (*get_attribute)(Display *, void *, int, int *) = dlsym(library, "glXGetFBConfigAttrib");
    if (!get_configs || !get_attribute) return 3;
    int count = 0;
    void **array = get_configs(display, DefaultScreen(display), &count);
    if (!array || count < 1) return 3;
    void *configuration = array[0];
    int before = 0, after = 0;
    if (get_attribute(display, configuration, 0x8010, &before)) return 3;
    XFree(array); /* GLX callers must be allowed to free this result array. */
    void *churn[128];
    for (int i = 0; i < 128; ++i) {
        churn[i] = malloc(240); /* Bundled Crusty's enumeration allocation. */
        if (!churn[i]) return 3;
        memset(churn[i], 0, 240);
    }
    int status = get_attribute(display, configuration, 0x8010, &after);
    printf("DRAWABLE-BEFORE=%d AFTER=%d\n", before, after);
    for (int i = 0; i < 128; ++i) free(churn[i]);
    XCloseDisplay(display);
    dlclose(library);
    return !status && before == 1 && after == 1 ? 0 : 4;
}
