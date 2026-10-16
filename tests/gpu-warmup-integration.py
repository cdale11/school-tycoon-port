"""Real GLES3 shader/pixel/state-restoration test on Mesa surfaceless EGL.
Optional argument: path to native libGLESv2.so.2 when not installed system-wide.
Requires Mesa EGL software rendering; no game assets or X server needed.
"""
import ctypes as c, ctypes.util, os, subprocess, sys, tempfile
from pathlib import Path
os.environ['EGL_PLATFORM']='surfaceless';os.environ['LIBGL_ALWAYS_SOFTWARE']='1'
r=Path(__file__).resolve().parents[1]
path=sys.argv[1] if len(sys.argv)>1 else ctypes.util.find_library('GLESv2')
if not path:raise SystemExit('Native GLES library required (pass its path)')
g=c.CDLL(path);os.environ['SDL_VIDEO_GL_DRIVER']=path
e=c.CDLL(ctypes.util.find_library('EGL'))
P=c.c_void_p;I=c.c_int;U=c.c_uint

def fn(lib,name,ret,args):
 f=getattr(lib,name);f.restype=ret;f.argtypes=args;return f
get=fn(e,'eglGetDisplay',P,[P]);init=fn(e,'eglInitialize',U,[P,P,P])
choose=fn(e,'eglChooseConfig',U,[P,P,P,I,P]);api=fn(e,'eglBindAPI',U,[U])
context=fn(e,'eglCreateContext',P,[P,P,P,P]);surface=fn(e,'eglCreatePbufferSurface',P,[P,P,P]);current=fn(e,'eglMakeCurrent',U,[P,P,P,P])
display=get(None);assert init(display,None,None)
a=(I*13)(0x3033,1,0x3040,64,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3038)
config=P();count=I();assert choose(display,a,c.byref(config),1,c.byref(count)) and count.value
assert api(0x30a0)
ctx=context(display,config,None,(I*3)(0x3098,3,0x3038));assert ctx
surf=surface(display,config,(I*5)(0x3057,8,0x3056,8,0x3038));assert surf
assert current(display,surf,surf,ctx)
getint=fn(g,'glGetIntegerv',None,[U,P]);enable=fn(g,'glEnable',None,[U]);isenabled=fn(g,'glIsEnabled',c.c_ubyte,[U])
pixelstore=fn(g,'glPixelStorei',None,[U,I]);viewport=fn(g,'glViewport',None,[I,I,I,I]);error=fn(g,'glGetError',U,[])
viewport(1,2,3,4);enable(0x0be2);enable(0x8c89)
for key,value in [(0x0d02,7),(0x0d03,2),(0x0d04,3),(0x0d05,8)]:pixelstore(key,value)
keys=[0x8b8d,0x8069,0x8caa,0x8ca6,0x88ed,0x88ef,0x85b5,0x0d02,0x0d03,0x0d04,0x0d05]
def snapshot():
 v=[]
 for key in keys:
  n=I();getint(key,c.byref(n));v.append(n.value)
 a=(I*4)();getint(0x0ba2,a);return v+list(a)+[isenabled(0x0be2),isenabled(0x8c89)]
expected=snapshot();assert error()==0
with tempfile.TemporaryDirectory(prefix='st-gpu-warmup-') as temp:
 lib=Path(temp)/'probe.so'
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-shared','-fPIC',str(r/'tools/st-gpu-warmup.c'),'-ldl','-o',str(lib)],check=True)
 probe=fn(c.CDLL(str(lib)),'st_gpu_warmup',I,[])
 for _ in range(3):
  assert probe()==1
  assert snapshot()==expected,'Native state changed'
  assert error()==0
fn(e,'eglMakeCurrent',U,[P,P,P,P])(display,None,None,None)
fn(e,'eglDestroyContext',U,[P,P])(display,ctx)
fn(e,'eglDestroySurface',U,[P,P])(display,surf)
fn(e,'eglTerminate',U,[P])(display)
print('PASS: real native shader/draw/readback, repeated cleanup and non-default state restoration')
