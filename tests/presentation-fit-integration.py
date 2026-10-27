"""Test production presentation coordinates, cursor alignment and GL state."""
from pathlib import Path
import os, subprocess, tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)
 (p/'mock.c').write_text(r'''
#include <assert.h>
#include <string.h>
int readfb=7,drawfb=9,scissor=1,clears,viewport[4]={0,0,256,1};
int destination,rect[4],width=720,height=480;
float color[4]={.2f,.3f,.4f,.5f};unsigned char mask[4]={0,1,0,1};
const unsigned char *glGetString(unsigned n){(void)n;return (void*)"OpenGL ES 3.2 fixture";}
void glGetIntegerv(unsigned n,int*p){if(n==0xba2)memcpy(p,viewport,sizeof(viewport));else *p=n==0x8caa?readfb:drawfb;}
void desktop_get(unsigned n,int*p){if(n==0xba2)memcpy(p,viewport,sizeof(viewport));else *p=n==0x8caa?1:destination;}
void glBindFramebuffer(unsigned n,unsigned v){if(n==0x8ca8)readfb=v;else drawfb=v;}
void glBlitFramebuffer(int a,int b,int c,int d,int x,int y,int z,int w,unsigned m,unsigned f){(void)a;(void)b;(void)c;(void)d;(void)m;(void)f;rect[0]=x;rect[1]=y;rect[2]=z;rect[3]=w;}
void glGetFloatv(unsigned n,float*p){(void)n;memcpy(p,color,sizeof(color));}
void glGetBooleanv(unsigned n,unsigned char*p){(void)n;memcpy(p,mask,sizeof(mask));}
unsigned char glIsEnabled(unsigned n){(void)n;return scissor;}
void glEnable(unsigned n){(void)n;scissor=1;}
void glDisable(unsigned n){(void)n;scissor=0;}
void glClearColor(float a,float b,float c,float d){color[0]=a;color[1]=b;color[2]=c;color[3]=d;}
void glColorMask(unsigned char a,unsigned char b,unsigned char c,unsigned char d){mask[0]=a;mask[1]=b;mask[2]=c;mask[3]=d;}
void glClear(unsigned n){assert(n==0x4000&&!scissor&&mask[0]&&mask[1]&&mask[2]&&mask[3]);assert(color[0]==0&&color[1]==0&&color[2]==0);clears++;}
void *SDL_GL_GetCurrentWindow(void){return (void*)1;}
void SDL_GL_GetDrawableSize(void*w,int*x,int*y){(void)w;*x=width;*y=height;}
int cursor_get_position(int*x,int*y){*x=400;*y=300;return 1;}
''')
 (p/'desktop.c').write_text('void desktop_get(unsigned,int*);void glGetIntegerv(unsigned n,int*p){desktop_get(n,p);}\n')
 (p/'main.c').write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include <dlfcn.h>
void glBlitFramebuffer(int,int,int,int,int,int,int,int,unsigned,unsigned);
int cursor_get_position(int*,int*);
int main(void){
 void*h=dlopen(getenv("SDL_VIDEO_GL_DRIVER"),RTLD_NOW|RTLD_GLOBAL);assert(h);
 #define V(n) (*(int*)dlsym(h,#n))
 int *r=dlsym(h,"rect");float *color=dlsym(h,"color");unsigned char *mask=dlsym(h,"mask");
 glBlitFramebuffer(0,0,800,600,0,0,800,600,0x4000,0x2600);
 assert(r[0]==0&&r[1]==0&&r[2]==720&&r[3]==480);
 assert(V(readfb)==7&&V(drawfb)==9&&V(scissor)==1&&V(clears)==0);
 assert(color[0]==.2f&&color[3]==.5f&&mask[0]==0&&mask[3]==1);
 int x,y;assert(cursor_get_position(&x,&y)&&x==360&&y==240);
 /* Wine reverses Y when copying into its on-screen drawable. */
 glBlitFramebuffer(0,0,800,600,0,600,800,0,0x4000,0x2600);
 assert(r[0]==0&&r[1]==480&&r[2]==720&&r[3]==0);
 V(clears)=0;
 V(destination)=2;glBlitFramebuffer(0,0,800,600,0,0,800,600,0x4000,0x2600);
 assert(r[2]==800&&r[3]==600&&V(clears)==0);
 V(destination)=0;glBlitFramebuffer(0,0,16,16,0,0,16,16,0x4000,0x2600);
 assert(r[2]==16&&r[3]==16&&V(clears)==0);
 V(width)=480;V(height)=720;
 glBlitFramebuffer(0,0,800,600,0,0,800,600,0x4000,0x2600);
 assert(r[0]==0&&r[1]==0&&r[2]==480&&r[3]==720);
 assert(cursor_get_position(&x,&y)&&x==240&&y==360);
 return 0;
}
''')
 for cmd in [
 ['cc','-shared','-fPIC',str(p/'mock.c'),'-o',str(p/'mock.so')],
 ['cc','-shared','-fPIC',str(p/'desktop.c'),'-L'+str(p),'-Wl,-rpath,'+str(p),'-l:mock.so','-o',str(p/'desktop.so')],
 ['cc','-Wall','-Wextra','-Werror','-ffunction-sections','-Wl,--gc-sections',str(repo/'tools/st-gl-present.c'),str(p/'main.c'),'-pthread','-ldl','-o',str(p/'test')]]:
  subprocess.run(cmd,check=True)
 # The production resolver requires loaded handles, so preload both libraries.
 env=dict(os.environ,LD_PRELOAD=str(p/'desktop.so')+':'+str(p/'mock.so'),BOX64_LIBGL=str(p/'desktop.so'),SDL_VIDEO_GL_DRIVER=str(p/'mock.so'),CRUSTY_LIBSDL=str(p/'mock.so'),ST_CRUSTY_LIBRARY=str(p/'mock.so'))
 subprocess.run([str(p/'test')],env=env,check=True,timeout=10)
 print('PASS: full-panel stretch, cursor coordinates, GL state, offscreen/partial copies')
