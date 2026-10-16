"""Exercise the production GLX wrappers: release, thread handoff, failed bind."""
from pathlib import Path
import os, subprocess, tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-context-') as directory:
 r=Path(directory)
 (r/'native.c').write_text(r'''
#include <stdlib.h>
static _Thread_local void *current;
static int calls;
void *eglGetDisplay(void *p){(void)p;return (void*)1;}
unsigned eglInitialize(void *p,int *a,int *b){(void)p;*a=1;*b=4;return 1;}
unsigned eglGetError(void){return 0;}
void *eglGetCurrentContext(void){return current;}
void *eglGetCurrentSurface(int s){(void)s;return current?(void*)3:0;}
void *SDL_CreateWindow(const char*t,int x,int y,int w,int h,unsigned f){
 (void)t;(void)x;(void)y;(void)w;(void)h;(void)f;return (void*)1;
}
void *SDL_GL_CreateContext(void*w){(void)w;return current=(void*)2;}
int SDL_GL_MakeCurrent(void *w,void*c){(void)w;if(getenv("ST_FAIL_BIND"))return -1;current=c;return 0;}
const char *SDL_GetError(void){return "simulated EGL_BAD_ACCESS";}
void *_SDL_CreateWindow(const char*,int,int,int,int,unsigned);
void *_SDL_GL_CreateContext(void*);
void init_backend(void){void*w=_SDL_CreateWindow("test",0,0,1,1,2);_SDL_GL_CreateContext(w);}
int crusty_glXMakeCurrent(void*d,unsigned long w,void*c){(void)d;(void)w;(void)c;calls++;return 1;}
int crusty_glXMakeContextCurrent(void*d,unsigned long w,unsigned long r,void*c){(void)d;(void)w;(void)r;(void)c;calls++;return 1;}
int bridge_calls(void){return calls;}
''')
 (r/'warmup.c').write_text('int st_gpu_warmup(void){return 1;}\n')
 (r/'main.c').write_text(r'''
#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
static int (*bind)(void*,unsigned long,void*);
static int (*bind_read)(void*,unsigned long,unsigned long,void*);
static void *(*current)(void);
static int (*calls)(void);
static void *worker(void *p){
 (void)p;assert(!current());
 assert(bind_read(0,1,1,(void*)8));assert(current()==(void*)2);
 assert(bind(0,0,0));assert(!current());return 0;
}
int main(void){
 void*h=dlopen(getenv("ST_CRUSTY_LIBRARY"),RTLD_NOW);
 bind=dlsym(RTLD_DEFAULT,"glXMakeCurrent");
 bind_read=dlsym(RTLD_DEFAULT,"glXMakeContextCurrent");
 current=dlsym(h,"eglGetCurrentContext");calls=dlsym(h,"bridge_calls");
 assert(bind&&bind_read&&current&&calls);assert(current()==(void*)2);
 assert(bind(0,0,0));assert(!current());
 pthread_t t;assert(!pthread_create(&t,0,worker,0));assert(!pthread_join(t,0));
 assert(!current());assert(bind(0,1,(void*)9));assert(current()==(void*)2);
 int old=calls();setenv("ST_FAIL_BIND","1",1);
 assert(!bind(0,0,0));assert(calls()==old);assert(current()==(void*)2);
 assert(!bind_read(0,1,1,(void*)9));assert(calls()==old);
 return 0;
}
''')
 for cmd in [
  ['cc','-shared','-fPIC',str(r/'native.c'),'-o',str(r/'native.so')],
  ['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(repo/'tools/st-egl-prime.c'),str(r/'warmup.c'),'-ldl','-o',str(r/'prime.so')],
  ['cc',str(r/'main.c'),'-pthread','-ldl','-o',str(r/'SchoolTycoon.exe')]]:
  subprocess.run(cmd,check=True)
 env=dict(os.environ,LD_PRELOAD=str(r/'prime.so'),CRUSTY_LIBEGL=str(r/'native.so'),CRUSTY_LIBSDL=str(r/'native.so'),ST_CRUSTY_LIBRARY=str(r/'native.so'),ST_EGL_TARGET='SchoolTycoon.exe')
 run=subprocess.run([str(r/'SchoolTycoon.exe')],env=env,capture_output=True,text=True,timeout=10)
 assert run.returncode==0,(run.returncode,run.stderr)
 assert 'simulated EGL_BAD_ACCESS' in run.stderr
 print('PASS: native release, cross-thread bind, GLX entry points, failure propagation')
