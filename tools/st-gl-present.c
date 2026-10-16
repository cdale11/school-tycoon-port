/* SPDX-License-Identifier: MIT
 * Use native GLES 3 framebuffer copies with GL4ES's framebuffer identifiers.
 * GL4ES retains the desktop API and all other rendering operations.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*blit_fn)(int, int, int, int, int, int, int, int, unsigned, unsigned);
static pthread_once_t symbols_once = PTHREAD_ONCE_INIT;
static pthread_once_t procedure_once = PTHREAD_ONCE_INIT;
static void *(*real_proc)(const unsigned char *);
static const unsigned char *(*native_string)(unsigned);
static void (*native_get)(unsigned, int *);
static void (*desktop_get)(unsigned, int *);
static void (*native_bind)(unsigned, unsigned);
static blit_fn native_blit, legacy_blit;
static pthread_mutex_t report_lock = PTHREAD_MUTEX_INITIALIZER;
static int reported;

static void *loaded_library(const char *variable, const char *fallback) {
    const char *path = getenv(variable);
    return dlopen(path && *path ? path : fallback, RTLD_NOW | RTLD_NOLOAD);
}

static void resolve_symbols(void) {
    void *desktop = loaded_library("BOX64_LIBGL", "libGL.so.1");
    void *native = loaded_library("SDL_VIDEO_GL_DRIVER", "/usr/lib/libGLESv2.so.2");
    if (desktop) {
        desktop_get = dlsym(desktop, "glGetIntegerv");
        legacy_blit = dlsym(desktop, "glBlitFramebuffer");
    }
    if (native) {
        native_string = dlsym(native, "glGetString");
        native_get = dlsym(native, "glGetIntegerv");
        native_bind = dlsym(native, "glBindFramebuffer");
        native_blit = dlsym(native, "glBlitFramebuffer");
    }
    /* Keep these handles alive with the native graphics session. */
}

static void resolve_procedure(void) {
    void *bridge = loaded_library("ST_CRUSTY_LIBRARY", "libcrusty.so");
    real_proc = bridge ? dlsym(bridge, "crusty_glXGetProcAddressARB") : NULL;
}

void glBlitFramebuffer(int sx0, int sy0, int sx1, int sy1,
                      int dx0, int dy0, int dx1, int dy1, unsigned mask, unsigned filter) {
    pthread_once(&symbols_once, resolve_symbols);
    const char *version = native_string ? (const char *)native_string(0x1f02) : NULL;
    int major = 0;
    if (version) sscanf(version, "OpenGL ES %d", &major);
    if (major >= 3 && native_blit && native_get && native_bind && desktop_get) {
        int source = 0, destination = 0, previous_source = 0, previous_destination = 0;
        desktop_get(0x8caa, &source);      /* GL_READ_FRAMEBUFFER_BINDING */
        desktop_get(0x8ca6, &destination); /* GL_DRAW_FRAMEBUFFER_BINDING */
        native_get(0x8caa, &previous_source);
        native_get(0x8ca6, &previous_destination);
        native_bind(0x8ca8, source);      /* GL_READ_FRAMEBUFFER */
        native_bind(0x8ca9, destination); /* GL_DRAW_FRAMEBUFFER */
        native_blit(sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, mask, filter);
        native_bind(0x8ca8, previous_source);
        native_bind(0x8ca9, previous_destination);
        pthread_mutex_lock(&report_lock);
        if (!reported) {
            fprintf(stderr, "ST graphics: native framebuffer copy through %s\n", version);
            reported = 1;
        }
        pthread_mutex_unlock(&report_lock);
    } else if (legacy_blit && legacy_blit != glBlitFramebuffer) {
        legacy_blit(sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, mask, filter);
    } else {
        fputs("ST graphics: no framebuffer copy implementation available\n", stderr);
        abort();
    }
}

void glBlitFramebufferEXT(int a, int b, int c, int d, int e, int f, int g, int h,
                         unsigned mask, unsigned filter) {
    glBlitFramebuffer(a, b, c, d, e, f, g, h, mask, filter);
}

void st_glLinkProgram(unsigned program);
void st_uniform_linked(unsigned program);
int st_glGetUniformLocation(unsigned, const char *);
void st_glUniform4fv(int, int, const float *);
void st_glUniformMatrix4fv(int, int, unsigned char, const float *);
void st_glDeleteProgram(unsigned);
void st_glUseProgram(unsigned);
void st_glShaderSource(unsigned, int, const char **, const int *);
void st_glCompileShader(unsigned);
void st_glGetShaderiv(unsigned, unsigned, int *);
void st_glGetShaderSource(unsigned, int, int *, char *);
void st_glDeleteShader(unsigned);
unsigned st_glCreateShader(unsigned);

static void link_with_uniforms(unsigned program) {
    st_glLinkProgram(program);
    st_uniform_linked(program);
}

void *crusty_glXGetProcAddressARB(const unsigned char *name) {
    pthread_once(&procedure_once, resolve_procedure);
    /* Preserve Crusty's backend initialization before returning an override. */
    void *result = real_proc && real_proc != crusty_glXGetProcAddressARB ? real_proc(name) : NULL;
    if (result && (!strcmp((const char *)name, "glXMakeCurrent")
               || !strcmp((const char *)name, "glXMakeContextCurrent"))) {
        void *binding=dlsym(RTLD_DEFAULT,(const char *)name);
        if (binding) return binding;
    }
    if (result && (!strcmp((const char *)name, "glBlitFramebuffer")
               || !strcmp((const char *)name, "glBlitFramebufferEXT")))
        return glBlitFramebuffer;
    if (result && (!strcmp((const char *)name, "glLinkProgram")
               || !strcmp((const char *)name, "glLinkProgramARB")))
        return link_with_uniforms;
    if (result && (!strcmp((const char *)name, "glGetUniformLocation")
               || !strcmp((const char *)name, "glGetUniformLocationARB")))
        return st_glGetUniformLocation;
    if (result && (!strcmp((const char *)name, "glUniform4fv")
               || !strcmp((const char *)name, "glUniform4fvARB")))
        return st_glUniform4fv;
    if (result && (!strcmp((const char *)name, "glUniformMatrix4fv")
               || !strcmp((const char *)name, "glUniformMatrix4fvARB")))
        return st_glUniformMatrix4fv;
    if (result && !strcmp((const char *)name, "glDeleteProgram"))
        return st_glDeleteProgram;
    if (result && (!strcmp((const char *)name, "glUseProgram") || !strcmp((const char *)name, "glUseProgramObjectARB"))) return st_glUseProgram;
    if (result && (!strcmp((const char *)name, "glShaderSource") || !strcmp((const char *)name, "glShaderSourceARB"))) return st_glShaderSource;
    if (result && (!strcmp((const char *)name, "glCompileShader") || !strcmp((const char *)name, "glCompileShaderARB"))) return st_glCompileShader;
    if (result && !strcmp((const char *)name, "glGetShaderiv")) return st_glGetShaderiv;
    if (result && (!strcmp((const char *)name, "glGetShaderSource") || !strcmp((const char *)name, "glGetShaderSourceARB"))) return st_glGetShaderSource;
    if (result && (!strcmp((const char *)name,"glCreateShader") || !strcmp((const char *)name,"glCreateShaderObjectARB"))) return st_glCreateShader;
    if (result && !strcmp((const char *)name, "glDeleteShader")) return st_glDeleteShader;
    return result;
}

void *crusty_glXGetProcAddress(const unsigned char *name) {
    return crusty_glXGetProcAddressARB(name);
}
