"""Model native surface failures the software X11 VM cannot produce itself.
Run the real preload constructor, including target selection and backend calls.
"""
from pathlib import Path
import os, subprocess, tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-native-start-') as tmp:
 r=Path(tmp)
 (r/'native.c').write_text(r'''
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
static int ready;
void *eglGetDisplay(void *p){(void)p; return (void*)1;}
unsigned eglInitialize(void *p,int *a,int *b){(void)p;*a=1;*b=4;return 1;}
unsigned eglGetError(void){return 0x3003;}
void *_SDL_CreateWindow(const char*,int,int,int,int,unsigned);
void *_SDL_GL_CreateContext(void*);
void *SDL_CreateWindow(const char*t,int x,int y,int w,int h,unsigned f){
 (void)t;(void)x;(void)y;(void)w;(void)h;(void)f;
 return getenv("ST_TEST_FAIL")?NULL:(void*)1;
}
void *SDL_GL_CreateContext(void*w){return w;}
const char *SDL_GetError(void){return "mali-fbdev: Can't create EGL window surface";}
void init_backend(void){
 void *window=_SDL_CreateWindow("test",0,0,720,480,2);
 ready=_SDL_GL_CreateContext(window)!=NULL;
}
void *eglGetCurrentContext(void){return ready?(void*)2:0;}
void *eglGetCurrentSurface(int s){return ready&&s==0x3059?(void*)3:0;}
''')
 (r/'main.c').write_text('#include <stdio.h>\nint main(void){puts("WINDOWS-MAPPINGS-START");return 0;}\n')
 for args in [ ['cc','-shared','-fPIC',str(r/'native.c'),'-o',str(r/'native.so')],
               ['cc','-Wall','-Wextra','-Werror','-shared','-fPIC',str(repo/'tools/st-egl-prime.c'),'-ldl','-o',str(r/'prime.so')],
               ['cc',str(r/'main.c'),'-o',str(r/'SchoolTycoon.exe')]]:
  subprocess.run(args,check=True)
 env=dict(os.environ,ST_EGL_TARGET='SchoolTycoon.exe',CRUSTY_LIBEGL=str(r/'native.so'),CRUSTY_LIBSDL=str(r/'native.so'),ST_CRUSTY_LIBRARY=str(r/'native.so'),LD_PRELOAD=str(r/'prime.so'))
 for fail in [False,True]:
  current=dict(env)
  if fail: current['ST_TEST_FAIL']='1'
  run=subprocess.run([str(r/'SchoolTycoon.exe')],env=current,capture_output=True,timeout=5)
  assert run.returncode==(1 if fail else 0),(run.returncode,run.stderr)
  assert (b'WINDOWS-MAPPINGS-START' in run.stdout)==(not fail)
  assert (b'ST-GRAPHICS-READY' in run.stderr)==(not fail)
  assert (b'ST-GRAPHICS-FAILED' in run.stderr)==fail
 # Wine helpers must never initialize or own the presentation window.
 run=subprocess.run([str(r/'SchoolTycoon.exe')],env=dict(env,ST_EGL_TARGET='another.exe',ST_TEST_FAIL='1'),capture_output=True,timeout=5)
 assert run.returncode==0 and b'ST graphics:' not in run.stderr
 print('PASS: complete native backend precedes Windows code; failed surface exits; helper processes untouched')
