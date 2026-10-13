/* Minimal EGL/GLES2 pbuffer test. No game files, display window, or system writes.
 * ABI types/constants below follow EGL 1.4 and OpenGL ES 2.0; dynamic loading
 * avoids linking against a different platform's graphics driver.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

typedef void *Display;
typedef void *Config;
typedef void *Surface;
typedef void *Context;
typedef unsigned int Boolean;
#define DECLARE(ret, name, args) typedef ret (*name##_fn) args; static name##_fn name
DECLARE(Display, eglGetDisplay, (void *));
DECLARE(Boolean, eglInitialize, (Display, int *, int *));
DECLARE(const char *, eglQueryString, (Display, int));
DECLARE(Boolean, eglBindAPI, (unsigned int));
DECLARE(Boolean, eglChooseConfig, (Display, const int *, Config *, int, int *));
DECLARE(Surface, eglCreatePbufferSurface, (Display, Config, const int *));
DECLARE(Context, eglCreateContext, (Display, Config, Context, const int *));
DECLARE(Boolean, eglMakeCurrent, (Display, Surface, Surface, Context));
DECLARE(Boolean, eglDestroySurface, (Display, Surface));
DECLARE(Boolean, eglDestroyContext, (Display, Context));
DECLARE(Boolean, eglTerminate, (Display));
DECLARE(int, eglGetError, (void));
DECLARE(const unsigned char *, glGetString, (unsigned int));
DECLARE(void, glClearColor, (float, float, float, float));
DECLARE(void, glClear, (unsigned int));
DECLARE(void, glFinish, (void));
DECLARE(void, glReadPixels, (int, int, int, int, unsigned int, unsigned int, void *));
DECLARE(unsigned int, glGetError, (void));
#define LOAD(lib, name) do { \
    name = (name##_fn)dlsym(lib, #name); \
    if (!name) { fprintf(stderr, "Missing symbol %s: %s\n", #name, dlerror()); return 1; } \
} while (0)

static const char *string_or_none(const char *value) {
    return value ? value : "(unavailable)";
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("School Tycoon native EGL/GLES2 rendering probe");
    void *egl = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!egl) egl = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (!egl) { fprintf(stderr, "Cannot load EGL: %s\n", dlerror()); return 1; }
    void *gles = dlopen("libGLESv2.so.2", RTLD_NOW | RTLD_LOCAL);
    if (!gles) gles = dlopen("libGLESv2.so", RTLD_NOW | RTLD_LOCAL);
    if (!gles) { fprintf(stderr, "Cannot load GLES2: %s\n", dlerror()); return 1; }
    LOAD(egl, eglGetDisplay); LOAD(egl, eglInitialize); LOAD(egl, eglQueryString);
    LOAD(egl, eglBindAPI); LOAD(egl, eglChooseConfig); LOAD(egl, eglCreatePbufferSurface);
    LOAD(egl, eglCreateContext); LOAD(egl, eglMakeCurrent); LOAD(egl, eglDestroySurface);
    LOAD(egl, eglDestroyContext); LOAD(egl, eglTerminate); LOAD(egl, eglGetError);
    LOAD(gles, glGetString); LOAD(gles, glClearColor); LOAD(gles, glClear);
    LOAD(gles, glFinish); LOAD(gles, glReadPixels); LOAD(gles, glGetError);

    Display display = eglGetDisplay(NULL);
    int major = 0, minor = 0;
    if (!display || !eglInitialize(display, &major, &minor)) {
        fprintf(stderr, "Default EGL display initialization failed: 0x%x\n", eglGetError());
        return 1;
    }
    printf("EGL version: %d.%d\n", major, minor);
    printf("EGL vendor: %s\n", string_or_none(eglQueryString(display, 0x3053)));
    printf("EGL APIs: %s\n", string_or_none(eglQueryString(display, 0x308d)));
    printf("EGL extensions: %s\n", string_or_none(eglQueryString(display, 0x3055)));
    Config config = NULL;
    Surface surface = NULL;
    Context context = NULL;
    int count = 0, result = 1;
    /* Pbuffer + GLES2; RGBA8 permits an unambiguous color readback. */
    const int attributes[] = {0x3033, 1, 0x3040, 4, 0x3024, 8, 0x3023, 8,
                              0x3022, 8, 0x3021, 8, 0x3038};
    const int dimensions[] = {0x3057, 16, 0x3056, 16, 0x3038};
    const int context_attributes[] = {0x3098, 2, 0x3038};
    if (!eglBindAPI(0x30a0) || !eglChooseConfig(display, attributes, &config, 1, &count) || count < 1) {
        fprintf(stderr, "No usable RGBA8 GLES2 pbuffer configuration: 0x%x\n", eglGetError());
        goto cleanup;
    }
    surface = eglCreatePbufferSurface(display, config, dimensions);
    context = eglCreateContext(display, config, NULL, context_attributes);
    if (!surface || !context || !eglMakeCurrent(display, surface, surface, context)) {
        fprintf(stderr, "Pbuffer/context creation failed: 0x%x\n", eglGetError());
        goto cleanup;
    }
    printf("GL vendor: %s\n", string_or_none((const char *)glGetString(0x1f00)));
    printf("GL renderer: %s\n", string_or_none((const char *)glGetString(0x1f01)));
    printf("GL version: %s\n", string_or_none((const char *)glGetString(0x1f02)));
    printf("GLSL version: %s\n", string_or_none((const char *)glGetString(0x8b8c)));
    unsigned char pixel[4] = {0, 0, 0, 0};
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(0x4000);
    glFinish();
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, pixel);
    unsigned int error = glGetError();
    printf("Readback RGBA: %u %u %u %u; GL error: 0x%x\n",
           pixel[0], pixel[1], pixel[2], pixel[3], error);
    if (error || abs((int)pixel[0] - 64) > 2 || abs((int)pixel[1] - 128) > 2 ||
        abs((int)pixel[2] - 191) > 2 || pixel[3] != 255) {
        fputs("Pixel readback failed.\n", stderr);
        goto cleanup;
    }
    puts("Native EGL/GLES2 rendering: PASS");
    result = 0;
cleanup:
    eglMakeCurrent(display, NULL, NULL, NULL);
    if (context) eglDestroyContext(display, context);
    if (surface) eglDestroySurface(display, surface);
    eglTerminate(display);
    dlclose(gles);
    dlclose(egl);
    return result;
}
