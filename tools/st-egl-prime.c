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
}
/* EGLInitialize is not reference-counted. SDL owns graphics teardown; early
 * priming must not terminate its shared display during DSO destruction. Keep
 * our library reference for process lifetime, reclaimed by the OS at exit. */
