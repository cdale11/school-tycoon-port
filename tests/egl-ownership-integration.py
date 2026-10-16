"""Priming must not terminate a shared EGL display during library teardown."""
from pathlib import Path
import os,subprocess,tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-egl-owner-') as d:
 root=Path(d)
 (root/'egl.c').write_text(r'''
#include <unistd.h>
static int initialized,terminated;
void *eglGetDisplay(void *native){(void)native;return (void*)1;}
unsigned eglInitialize(void *display,int *major,int *minor){(void)display;initialized=1;*major=1;*minor=5;return 1;}
unsigned eglGetError(void){return 0;}
unsigned eglTerminate(void *display){(void)display;terminated++;return 1;}
__attribute__((destructor)) static void check(void){if(!initialized||terminated)_exit(87);}
''')
 (root/'main.c').write_text('int main(void){return 0;}\n')
 for command in [['cc','-shared','-fPIC',str(root/'egl.c'),'-o',str(root/'egl.so')],['cc','-shared','-fPIC',str(repo/'tools/st-egl-prime.c'),str(repo/'tools/st-gpu-warmup.c'),'-ldl','-o',str(root/'prime.so')],['cc',str(root/'main.c'),'-o',str(root/'SchoolTycoon.exe')]]:
  subprocess.run(command,check=True)
 env=dict(os.environ,ST_EGL_TARGET='SchoolTycoon.exe',CRUSTY_LIBEGL=str(root/'egl.so'),LD_PRELOAD=str(root/'prime.so'))
 result=subprocess.run([str(root/'SchoolTycoon.exe')],env=env,capture_output=True,timeout=10)
 assert result.returncode==0,(result.returncode,result.stderr)
 assert b'initialized before Windows mappings' in result.stderr
 print('PASS: native EGL primed without terminating the shared client display at exit')
