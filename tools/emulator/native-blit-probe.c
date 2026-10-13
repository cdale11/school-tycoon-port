/* SPDX-License-Identifier: MIT
 * Exercise the real GL4ES/GLES copy and verify native binding restoration.
 * Run in the prepared ARM64 VM with the presentation helper preloaded.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define LOAD(handle, result, name, parameters) \
    result (*name) parameters = dlsym(handle, #name); if (!name) return 2

int main(void) {
    void *desktop = dlopen(getenv("BOX64_LIBGL"), RTLD_NOW | RTLD_GLOBAL);
    if (!desktop) return 1;
    Display *display = XOpenDisplay(NULL);
    if (!display) return 1;
    LOAD(desktop, XVisualInfo *, glXChooseVisual, (Display *, int, int *));
    LOAD(desktop, void *, glXCreateContext, (Display *, XVisualInfo *, void *, int));
    LOAD(desktop, int, glXMakeCurrent, (Display *, unsigned long, void *));
    LOAD(desktop, void *, glXGetProcAddressARB, (const unsigned char *));
    XVisualInfo *visual = glXChooseVisual(display, DefaultScreen(display), NULL);
    if (!visual) return 2;
    void *context = glXCreateContext(display, visual, NULL, 1);
    if (!context || !glXMakeCurrent(display, DefaultRootWindow(display), context)) return 2;

    LOAD(desktop, void, glGenTextures, (int, unsigned *));
    LOAD(desktop, void, glBindTexture, (unsigned, unsigned));
    LOAD(desktop, void, glTexImage2D, (unsigned, int, int, int, int, int, unsigned, unsigned, const void *));
    LOAD(desktop, void, glGenFramebuffers, (int, unsigned *));
    LOAD(desktop, void, glBindFramebuffer, (unsigned, unsigned));
    LOAD(desktop, void, glFramebufferTexture2D, (unsigned, unsigned, unsigned, unsigned, int));
    LOAD(desktop, void, glClearColor, (float, float, float, float));
    LOAD(desktop, void, glClear, (unsigned));
    unsigned texture = 0, framebuffer = 0;
    glGenTextures(1, &texture);
    glBindTexture(0x0de1, texture);
    glTexImage2D(0x0de1, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, NULL);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(0x8d40, framebuffer);
    glFramebufferTexture2D(0x8d40, 0x8ce0, 0x0de1, texture, 0);
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(0x4000);
    glBindFramebuffer(0x8ca8, framebuffer);
    glBindFramebuffer(0x8ca9, 0);

    void *native = dlopen(getenv("SDL_VIDEO_GL_DRIVER"), RTLD_NOW | RTLD_NOLOAD);
    if (!native) return 2;
    void (*get)(unsigned, int *) = dlsym(native, "glGetIntegerv");
    void (*bind)(unsigned, unsigned) = dlsym(native, "glBindFramebuffer");
    void (*read)(int, int, int, int, unsigned, unsigned, void *) = dlsym(native, "glReadPixels");
    if (!get || !bind || !read) return 2;
    int before_read = 0, before_draw = 0, after_read = 0, after_draw = 0;
    get(0x8caa, &before_read); get(0x8ca6, &before_draw);
    void (*blit)(int, int, int, int, int, int, int, int, unsigned, unsigned) =
        glXGetProcAddressARB((const unsigned char *)"glBlitFramebuffer");
    if (!blit) return 2;
    blit(0, 0, 16, 16, 0, 0, 64, 64, 0x4000, 0x2600);
    get(0x8caa, &after_read); get(0x8ca6, &after_draw);
    if (before_read != after_read || before_draw != after_draw) return 3;
    unsigned char pixel[4] = {0};
    bind(0x8ca8, 0);
    read(8, 8, 1, 1, 0x1908, 0x1401, pixel);
    bind(0x8ca8, before_read);
    printf("Native copy pixel: %u %u %u %u\n", pixel[0], pixel[1], pixel[2], pixel[3]);
    if (abs(pixel[0] - 64) > 1 || abs(pixel[1] - 128) > 1 || abs(pixel[2] - 191) > 1 || pixel[3] != 255)
        return 4;
    puts("ST-NATIVE-BLIT-PASS: native framebuffer bindings restored");
    return 0;
}
