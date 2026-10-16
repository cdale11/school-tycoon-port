/* SPDX-License-Identifier: MIT
 * Exercise the real GL4ES/GLES copy and verify native binding restoration.
 * Run in the prepared ARM64 VM with the presentation helper preloaded.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOAD(handle, result, name, parameters) \
    result (*name) parameters = dlsym(handle, #name); if (!name) return 2

struct handoff {
    Display *display;
    void *context;
    int (*bind)(Display *,unsigned long,void *);
    void *(*native_current)(void);
    int passed;
};
static void *check_handoff(void *argument) {
    struct handoff *h=argument;
    if (h->native_current()) return NULL;
    if (!h->bind(h->display,DefaultRootWindow(h->display),h->context)) return NULL;
    if (!h->native_current()) return NULL;
    if (!h->bind(h->display,0,NULL) || h->native_current()) return NULL;
    h->passed=1;
    return NULL;
}

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

    if (getenv("ST_EXPECT_CONTEXT_HANDOFF")) {
        void *egl=dlopen(getenv("CRUSTY_LIBEGL"),RTLD_NOW|RTLD_NOLOAD);
        void *(*current)(void)=egl?dlsym(egl,"eglGetCurrentContext"):NULL;
        if (!current || !current()) return 8;
        if (!glXMakeCurrent(display,0,NULL) || current()) return 9;
        struct handoff h={display,context,glXMakeCurrent,current,0};
        pthread_t thread;
        if (pthread_create(&thread,NULL,check_handoff,&h)) return 10;
        if (pthread_join(thread,NULL) || !h.passed) return 11;
        if (!glXMakeCurrent(display,DefaultRootWindow(display),context) || !current()) return 12;
        puts("ST-CONTEXT-HANDOFF-PASS: native EGL release and cross-thread binding");
    }

    if (getenv("ST_EXPECT_GLSL_ONLY")) {
        LOAD(desktop, const unsigned char *, glGetString, (unsigned));
        const char *extensions = (const char *)glGetString(0x1f03);
        if (!extensions || strstr(extensions, "GL_ARB_fragment_program ")
            || strstr(extensions, "GL_ARB_vertex_program ")) return 5;
        LOAD(desktop, unsigned, glCreateShader, (unsigned));
        LOAD(desktop, void, glShaderSource, (unsigned, int, const char **, const int *));
        LOAD(desktop, void, glCompileShader, (unsigned));
        LOAD(desktop, void, glGetShaderiv, (unsigned, unsigned, int *));
        LOAD(desktop, unsigned, glCreateProgram, (void));
        LOAD(desktop, void, glAttachShader, (unsigned, unsigned));
        LOAD(desktop, void, glLinkProgram, (unsigned));
        LOAD(desktop, void, glGetProgramiv, (unsigned, unsigned, int *));
        LOAD(desktop, void, glDeleteShader, (unsigned));
        LOAD(desktop, void, glDeleteProgram, (unsigned));
        const char *source[2] = {
            "#version 120\nvoid main(){gl_Position=gl_Vertex;}",
            "#version 120\nvoid main(){gl_FragColor=vec4(0.25,0.5,0.75,1.0);}"};
        unsigned shader[2], program = glCreateProgram();
        for (int i=0; i<2; i++) {
            shader[i]=glCreateShader(i ? 0x8b30 : 0x8b31);
            glShaderSource(shader[i],1,&source[i],NULL); glCompileShader(shader[i]);
            int compiled=0; glGetShaderiv(shader[i],0x8b81,&compiled);
            if (!compiled) return 6;
            glAttachShader(program,shader[i]);
        }
        glLinkProgram(program);
        int linked=0; glGetProgramiv(program,0x8b82,&linked);
        glDeleteProgram(program);
        for (int i=0;i<2;i++) glDeleteShader(shader[i]);
        if (!linked) return 7;
        puts("ST-GLSL-PASS: ARB assembly extensions absent; desktop GLSL links");
    }

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
