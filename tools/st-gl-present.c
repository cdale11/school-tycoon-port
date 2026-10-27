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
static void *(*sdl_window)(void);
static void (*sdl_size)(void *,int *,int *);
static unsigned char (*native_enabled)(unsigned);
static void (*native_enable)(unsigned), (*native_disable)(unsigned);
static int logical_width,logical_height,fit_x,fit_y,fit_width,fit_height;
static int (*real_cursor_position)(int *,int *);

static void *loaded_library(const char *variable, const char *fallback) {
    const char *path = getenv(variable);
    return dlopen(path && *path ? path : fallback, RTLD_NOW | RTLD_NOLOAD);
}

static void resolve_symbols(void) {
    void *desktop = loaded_library("BOX64_LIBGL", "libGL.so.1");
    void *native = loaded_library("SDL_VIDEO_GL_DRIVER", "/usr/lib/libGLESv2.so.2");
    void *sdl = loaded_library("CRUSTY_LIBSDL", "/usr/lib/libSDL2-2.0.so.0");
    if(sdl) {
        sdl_window=dlsym(sdl,"SDL_GL_GetCurrentWindow");
        sdl_size=dlsym(sdl,"SDL_GL_GetDrawableSize");
    }
    if (desktop) {
        desktop_get = dlsym(desktop, "glGetIntegerv");
        legacy_blit = dlsym(desktop, "glBlitFramebuffer");
    }
    if (native) {
        native_string = dlsym(native, "glGetString");
        native_get = dlsym(native, "glGetIntegerv");
        native_bind = dlsym(native, "glBindFramebuffer");
        native_blit = dlsym(native, "glBlitFramebuffer");
        native_enabled=dlsym(native,"glIsEnabled");
        native_enable=dlsym(native,"glEnable");
        native_disable=dlsym(native,"glDisable");
    }
    /* Keep these handles alive with the native graphics session. */
}

static void resolve_procedure(void) {
    void *bridge = loaded_library("ST_CRUSTY_LIBRARY", "libcrusty.so");
    real_proc = bridge ? dlsym(bridge, "crusty_glXGetProcAddressARB") : NULL;
    real_cursor_position=bridge?dlsym(bridge,"cursor_get_position"):NULL;
}

/* Crusty's cursor is a native overlay, while XTest/Wine use game coordinates.
 * Transform only the overlay; never change the coordinates seen by the game. */
int cursor_get_position(int *x,int *y) {
    pthread_once(&procedure_once,resolve_procedure);
    if(!real_cursor_position || real_cursor_position==cursor_get_position) return 0;
    int ok=real_cursor_position(x,y);
    pthread_mutex_lock(&report_lock);
    if(ok && logical_width>0 && logical_height>0) {
        *x=fit_x+(int)((long long)*x*fit_width/logical_width);
        *y=fit_y+(int)((long long)*y*fit_height/logical_height);
    }
    pthread_mutex_unlock(&report_lock);
    return ok;
}

/* Isolate the native overlay from GL4ES's cached game state. In particular,
 * Wine can leave a 256x1 viewport, subtract blending, a sampler, or unpack
 * buffer/row offsets active. These must not affect the cursor texture/draw. */
static pthread_once_t cursor_once=PTHREAD_ONCE_INIT;
static void (*draw_cursor)(void);
static void (*cg)(unsigned,int*), (*active)(unsigned), (*texture)(unsigned,unsigned);
static void (*buffer)(unsigned,unsigned), (*vao)(unsigned), (*program)(unsigned);
static void (*viewport_fn)(int,int,int,int), (*equation)(unsigned,unsigned);
static void (*blend_fn)(unsigned,unsigned,unsigned,unsigned);
static void (*color_mask)(unsigned char,unsigned char,unsigned char,unsigned char);
static void (*pixel_store)(unsigned,int), (*sampler)(unsigned,unsigned);
static int cursor_ready;
static void resolve_cursor(void) {
    void *n=loaded_library("SDL_VIDEO_GL_DRIVER","/usr/lib/libGLESv2.so.2");
    void *b=loaded_library("ST_CRUSTY_LIBRARY","libcrusty.so");
    if(!n || !b) return;
    draw_cursor=dlsym(b,"cursor_draw");
#define C(var,name) var=dlsym(n,name); if(!var) return
    C(cg,"glGetIntegerv"); C(active,"glActiveTexture"); C(texture,"glBindTexture");
    C(buffer,"glBindBuffer"); C(vao,"glBindVertexArray"); C(program,"glUseProgram");
    C(viewport_fn,"glViewport"); C(equation,"glBlendEquationSeparate");
    C(blend_fn,"glBlendFuncSeparate"); C(color_mask,"glColorMask");
    C(pixel_store,"glPixelStorei"); C(sampler,"glBindSampler");
#undef C
    cursor_ready=1;
}
void cursor_draw(void) {
    pthread_once(&symbols_once,resolve_symbols);
    pthread_once(&cursor_once,resolve_cursor);
    if(!draw_cursor || draw_cursor==cursor_draw) return;
    if(!cursor_ready || !native_enabled || !native_enable || !native_disable || !sdl_window || !sdl_size) { draw_cursor(); return; }
    int width=0,height=0; void *window=sdl_window();
    if(window) sdl_size(window,&width,&height);
    if(width<=0 || height<=0) return;
    /* Save the original active unit before forcing all overlay work to unit 0. */
    int unit; cg(0x84e0,&unit); active(0x84c0);
    const unsigned names[]={0x8069,0x8919,0x85b5,0x8894,0x8b8d,
        0x8009,0x883d,0x80c9,0x80c8,0x80cb,0x80ca,
        0x88ef,0x0cf5,0x0cf2,0x0cf3,0x0cf4};
    int saved[16],vp[4],mask[4];
    for(unsigned i=0;i<16;i++) cg(names[i],&saved[i]);
    cg(0x0ba2,vp);cg(0x0c23,mask);
    const unsigned caps[]={0x0be2,0x0b71,0x0c11,0x0b90,0x0b44,0x8c89,0x809e,0x80a0};
    unsigned char enabled[8];
    for(unsigned i=0;i<8;i++) {enabled[i]=native_enabled(caps[i]);native_disable(caps[i]);}
    viewport_fn(0,0,width,height);color_mask(1,1,1,1);
    equation(0x8006,0x8006);sampler(0,0);
    buffer(0x88ec,0);pixel_store(0x0cf5,4);
    pixel_store(0x0cf2,0);pixel_store(0x0cf3,0);pixel_store(0x0cf4,0);
    draw_cursor();
    static int cursor_reported;
    pthread_mutex_lock(&report_lock);
    if(!cursor_reported) {
        fprintf(stderr,"ST cursor: isolated native overlay %dx%d; restored game viewport %dx%d\n",width,height,vp[2],vp[3]);
        cursor_reported=1;
    }
    pthread_mutex_unlock(&report_lock);
    active(0x84c0);texture(0x0de1,saved[0]);sampler(0,saved[1]);
    vao(saved[2]);buffer(0x8892,saved[3]);program(saved[4]);
    equation(saved[5],saved[6]);blend_fn(saved[7],saved[8],saved[9],saved[10]);
    buffer(0x88ec,saved[11]);
    for(unsigned i=12;i<16;i++) pixel_store(names[i],saved[i]);
    viewport_fn(vp[0],vp[1],vp[2],vp[3]);color_mask(mask[0],mask[1],mask[2],mask[3]);
    for(unsigned i=0;i<8;i++) {if(enabled[i]) native_enable(caps[i]);else native_disable(caps[i]);}
    active(unit);
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
        int fitted=0;
        unsigned char scissor=0;
        /* Only a full-window color presentation is resized. Offscreen and
         * partial copies keep their original coordinates and state. */
        int viewport[4]={0}; desktop_get(0x0ba2,viewport);
        int left=dx0<dx1?dx0:dx1, bottom=dy0<dy1?dy0:dy1;
        int right=dx0>dx1?dx0:dx1, top=dy0>dy1?dy0:dy1;
        int reverse_x=dx0>dx1, reverse_y=dy0>dy1;
        if(!destination && source && mask==0x4000 && !left && !bottom &&
           /* School Tycoon's supported 800x600 mode. Wine leaves a 256x1
            * viewport after its capability probes; blits ignore the viewport.
            * Match the complete source and destination instead. */
           right==800 && top==600 && !sx0 && !sy0 && sx1==800 && sy1==600 &&
           sdl_window && sdl_size &&
           native_enabled && native_enable && native_disable) {
            void *window=sdl_window(); int width=0,height=0;
            if(window) sdl_size(window,&width,&height);
            if(width>0 && height>0) {
                /* Fill the physical panel; the user prefers stretching over bars. */
                int fw=width,fh=height;
                if(fw>0 && fh>0) {
                    pthread_mutex_lock(&report_lock);
                    if(logical_width!=right || logical_height!=top || fit_width!=fw || fit_height!=fh)
                        fprintf(stderr,"ST display: %dx%d -> %dx%d at %d,%d on %dx%d\n",right,top,fw,fh,(width-fw)/2,(height-fh)/2,width,height);
                    logical_width=right; logical_height=top;
                    fit_x=(width-fw)/2;fit_y=(height-fh)/2;fit_width=fw;fit_height=fh;
                    dx0=fit_x+(reverse_x?fw:0);dx1=fit_x+(reverse_x?0:fw);
                    dy0=fit_y+(reverse_y?fh:0);dy1=fit_y+(reverse_y?0:fh);
                    pthread_mutex_unlock(&report_lock);
                    scissor=native_enabled(0x0c11);
                    if(scissor) native_disable(0x0c11);
                    /* Full-panel copy overwrites every pixel: no bar clear. */
                    fitted=1;
                }
            }
        }
        native_blit(sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, mask, filter);
        if(fitted) {
            if(scissor) native_enable(0x0c11);
        }
        native_bind(0x8ca8, previous_source);
        native_bind(0x8ca9, previous_destination);
        pthread_mutex_lock(&report_lock);
        if (!reported) {
            fprintf(stderr,"ST blit: src=%d dst=%d source=%d,%d,%d,%d destination=%d,%d,%d,%d viewport=%d,%d,%d,%d\n",source,destination,sx0,sy0,sx1,sy1,dx0,dy0,dx1,dy1,viewport[0],viewport[1],viewport[2],viewport[3]);
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
