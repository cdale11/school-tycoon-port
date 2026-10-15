/* SPDX-License-Identifier: MIT
 * Load the selected native EGL/SDL libraries directly for Crusty, avoiding its
 * persistent /tmp aliases. Prime EGL before Wine creates Windows VA mappings.
 * This library is preloaded only into the application's Box64/Wine invocation.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *egl_library;
static void *sdl_library;
static void fail_early(void) { fflush(stderr); _exit(1); }
static int matching_target(void) {
    const char *target=getenv("ST_EGL_TARGET");
    if(!target || !*target) return 0;
    FILE *f=fopen("/proc/self/cmdline","rb");
    if(!f) return 0;
    char args[8192];
    size_t n=fread(args,1,sizeof(args)-1,f);
    fclose(f); args[n]=0;
    for(size_t i=0;i<n;) {
        const char *arg=args+i;
        const char *base=strrchr(arg,'/');
        base=base?base+1:arg;
        const char *winbase=strrchr(base,'\\');
        if(winbase) base=winbase+1;
        if(!strcasecmp(base,target)) return 1;
        i+=strlen(arg)+1;
    }
    return 0;
}
/* Crusty calls this exported entry point through its PLT for EGL and SDL.
 * Keep its API but bypass the fixed /tmp/CRUSTY_LIB*64.so alias mechanism. */
void *load_lib_local(const char *fallback,const char *variable) {
    const char *path=getenv(variable);
    if(!path || !*path) path=fallback;
    void *handle=dlopen(path,RTLD_NOW|RTLD_LOCAL);
    if(!handle) fprintf(stderr,"ST graphics: cannot load %s: %s\n",path,dlerror());
    else fprintf(stderr,"ST graphics: native %s loaded from %s\n",variable,path);
    return handle;
}
static void *native_sdl_symbol(const char *name) {
    if(!sdl_library) sdl_library=load_lib_local("/usr/lib/libSDL2-2.0.so.0","CRUSTY_LIBSDL");
    void *symbol=sdl_library?dlsym(sdl_library,name):NULL;
    if(!symbol) { fprintf(stderr,"ST-GRAPHICS-FAILED: missing native %s\n",name); fail_early(); }
    return symbol;
}
static void check_sdl_result(void *result,const char *operation) {
    if(result) return;
    const char *(*error)(void)=native_sdl_symbol("SDL_GetError");
    fprintf(stderr,"ST-GRAPHICS-FAILED: %s: %s\n",operation,error());
    fail_early();
}
/* The pinned bridge calls these exports through its PLT. Stop at the original
 * failure, before it tries GL4ES hardware discovery with a NULL context. */
void *_SDL_CreateWindow(const char *title,int x,int y,int w,int h,unsigned flags) {
    void *(*create)(const char *,int,int,int,int,unsigned)=native_sdl_symbol("SDL_CreateWindow");
    void *window=create(title,x,y,w,h,flags);
    check_sdl_result(window,"SDL_CreateWindow");
    return window;
}
void *_SDL_GL_CreateContext(void *window) {
    void *(*create)(void *)=native_sdl_symbol("SDL_GL_CreateContext");
    void *context=create(window);
    check_sdl_result(context,"SDL_GL_CreateContext");
    return context;
}
__attribute__((constructor)) static void prime_egl(void) {
    if(!matching_target()) return;
    const char *path=getenv("CRUSTY_LIBEGL");
    if(!path || !*path) path="/usr/lib/libEGL.so.1";
    egl_library=dlopen(path,RTLD_NOW|RTLD_LOCAL);
    if(!egl_library) { fprintf(stderr,"ST graphics: early EGL load failed: %s\n",dlerror()); fail_early(); }
    void *(*get_display)(void *)=dlsym(egl_library,"eglGetDisplay");
    unsigned (*initialize)(void *,int *,int *)=dlsym(egl_library,"eglInitialize");
    unsigned (*get_error)(void)=dlsym(egl_library,"eglGetError");
    if(!get_display || !initialize || !get_error) {
        fputs("ST graphics: native EGL entry points missing\n",stderr); fail_early();
    }
    void *display=get_display(NULL);
    int major=0,minor=0;
    if(!display || !initialize(display,&major,&minor)) {
        fprintf(stderr,"ST graphics: early EGL initialization failed (0x%x)\n",get_error()); fail_early();
    }
    fprintf(stderr,"ST graphics: EGL %d.%d initialized before Windows mappings\n",major,minor);
    /* Initializing EGL alone does not allocate the native window buffers.
     * Complete the pinned Crusty backend before Wine reserves low addresses:
     * Mali/ION surface allocations also need native address space. Reuse this
     * same backend for GLX; never create a second, temporary SDL window. */
    const char *bridge_path=getenv("ST_CRUSTY_LIBRARY");
    if(!bridge_path || !*bridge_path) return; /* EGL-only diagnostic */
    void *bridge=dlopen(bridge_path,RTLD_NOW|RTLD_NOLOAD);
    void (*backend_init)(void)=bridge?dlsym(bridge,"init_backend"):NULL;
    void *(*current_context)(void)=dlsym(egl_library,"eglGetCurrentContext");
    void *(*current_surface)(int)=dlsym(egl_library,"eglGetCurrentSurface");
    if(!backend_init || !current_context || !current_surface) {
        fputs("ST-GRAPHICS-FAILED: native backend entry points missing\n",stderr);
        fail_early();
    }
    backend_init();
    if(!current_context() || !current_surface(0x3059)) { /* EGL_DRAW */
        fprintf(stderr,"ST-GRAPHICS-FAILED: no native context/draw surface (EGL 0x%x)\n",get_error());
        fail_early();
    }
    fputs("ST-GRAPHICS-READY: native surface and context created before Windows mappings\n",stderr);
}
/* EGLInitialize is not reference-counted. SDL owns graphics teardown; early
 * priming must not terminate its shared display during DSO destruction. Keep
 * our library reference for process lifetime, reclaimed by the OS at exit. */
