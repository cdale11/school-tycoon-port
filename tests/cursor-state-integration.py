"""Crusty's native cursor must neither inherit nor leak game GL state."""
from pathlib import Path
import os, subprocess, tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)
 (p/'mock.c').write_text(r'''
#include <assert.h>
#include <string.h>
int state[65536],before[65536],vp[4]={0,0,256,1},mask[4]={0,1,0,1},calls;
void glGetIntegerv(unsigned n,int*p){if(n==0xba2)memcpy(p,vp,sizeof(vp));else if(n==0xc23)memcpy(p,mask,sizeof(mask));else *p=state[n];}
void glActiveTexture(unsigned v){state[0x84e0]=v;}
void glBindTexture(unsigned t,unsigned v){(void)t;state[0x8069]=v;}
void glBindSampler(unsigned t,unsigned v){(void)t;state[0x8919]=v;}
void glBindBuffer(unsigned t,unsigned v){state[t==0x8892?0x8894:0x88ef]=v;}
void glBindVertexArray(unsigned v){state[0x85b5]=v;}
void glUseProgram(unsigned v){state[0x8b8d]=v;}
void glViewport(int a,int b,int c,int d){vp[0]=a;vp[1]=b;vp[2]=c;vp[3]=d;}
void glBlendEquationSeparate(unsigned a,unsigned b){state[0x8009]=a;state[0x883d]=b;}
void glBlendFuncSeparate(unsigned a,unsigned b,unsigned c,unsigned d){state[0x80c9]=a;state[0x80c8]=b;state[0x80cb]=c;state[0x80ca]=d;}
void glColorMask(unsigned char a,unsigned char b,unsigned char c,unsigned char d){mask[0]=a;mask[1]=b;mask[2]=c;mask[3]=d;}
void glPixelStorei(unsigned n,int v){state[n]=v;}
unsigned char glIsEnabled(unsigned n){return state[n]!=0;}
void glEnable(unsigned n){state[n]=1;}
void glDisable(unsigned n){state[n]=0;}
void *SDL_GL_GetCurrentWindow(void){return (void*)1;}
void SDL_GL_GetDrawableSize(void*w,int*x,int*y){(void)w;*x=720;*y=480;}
void cursor_draw(void){
 assert(vp[2]==720&&vp[3]==480&&mask[0]&&mask[2]);
 assert(state[0x8009]==0x8006&&state[0x883d]==0x8006);
 assert(state[0x84e0]==0x84c0&&!state[0x8919]&&!state[0x88ef]);
 assert(state[0xcf5]==4&&!state[0xcf2]&&!state[0xcf3]&&!state[0xcf4]);
 assert(!state[0xc11]&&!state[0xb90]&&!state[0xb44]&&!state[0x8c89]);
 glBindTexture(0xde1,888);glBindVertexArray(0);glBindBuffer(0x8892,0);
 glUseProgram(567);glBlendFuncSeparate(0x302,0x303,0x302,0x303);glEnable(0xbe2);
 calls++;
}
void init(void){
 unsigned caps[]={0xbe2,0xb71,0xc11,0xb90,0xb44,0x8c89,0x809e,0x80a0};
 unsigned fields[]={0x8069,0x8919,0x85b5,0x8894,0x8b8d,0x8009,0x883d,0x80c9,0x80c8,0x80cb,0x80ca,0x88ef,0xcf5,0xcf2,0xcf3,0xcf4};
 for(unsigned i=0;i<16;i++)state[fields[i]]=i+10;
 for(unsigned i=0;i<8;i++)state[caps[i]]=i%2;
 state[0x84e0]=0x84c3;memcpy(before,state,sizeof(state));
}
void check(void){assert(!memcmp(state,before,sizeof(state)));assert(vp[2]==256&&vp[3]==1&&!mask[0]&&mask[1]&&!mask[2]&&mask[3]);assert(calls==2);}
''')
 (p/'main.c').write_text('''#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
void cursor_draw(void);
int main(void){void*h=dlopen(getenv("ST_CRUSTY_LIBRARY"),RTLD_NOW);assert(h);((void(*)(void))dlsym(h,"init"))();cursor_draw();cursor_draw();((void(*)(void))dlsym(h,"check"))();}
''')
 subprocess.run(['cc','-shared','-fPIC',str(p/'mock.c'),'-o',str(p/'mock.so')],check=True)
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-ffunction-sections','-Wl,--gc-sections',str(repo/'tools/st-gl-present.c'),str(p/'main.c'),'-pthread','-ldl','-o',str(p/'test')],check=True)
 env=dict(os.environ,LD_PRELOAD=str(p/'mock.so'),SDL_VIDEO_GL_DRIVER=str(p/'mock.so'),CRUSTY_LIBSDL=str(p/'mock.so'),ST_CRUSTY_LIBRARY=str(p/'mock.so'))
 subprocess.run([str(p/'test')],env=env,check=True,timeout=10)
 print('PASS: cursor viewport, blending, sampler, upload state and complete restoration across repeated draws')
