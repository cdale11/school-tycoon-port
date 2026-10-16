/* SPDX-License-Identifier: MIT
 * Initialize lazy native GPU/compiler resources before Wine VA reservations.
 * Exercise a real draw/readback, restoring all native state changed here.
 * No vendor code, allocation hooks or application texture settings are changed.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FN(ret,name,args) ret (*name) args=dlsym(lib,#name); if(!name){fprintf(stderr,"ST-NATIVE-WARMUP-FAILED: missing %s\n",#name);return 0;}
int st_gpu_warmup(void) {
 const char *path=getenv("SDL_VIDEO_GL_DRIVER");
 void *lib=path?dlopen(path,RTLD_NOW|RTLD_NOLOAD):NULL;
 if(!lib){fputs("ST-NATIVE-WARMUP-FAILED: native GLES not loaded\n",stderr);return 0;}
 FN(const unsigned char*,glGetString,(unsigned));
 FN(unsigned,glGetError,(void));
 FN(void,glGetIntegerv,(unsigned,int*));
 FN(void,glGetBooleanv,(unsigned,unsigned char*));
 FN(unsigned char,glIsEnabled,(unsigned));
 FN(void,glEnable,(unsigned)); FN(void,glDisable,(unsigned));
 FN(void,glColorMask,(unsigned char,unsigned char,unsigned char,unsigned char));
 FN(void,glViewport,(int,int,int,int));
 FN(unsigned,glCreateShader,(unsigned));
 FN(void,glShaderSource,(unsigned,int,const char**,const int*));
 FN(void,glCompileShader,(unsigned));
 FN(void,glGetShaderiv,(unsigned,unsigned,int*));
 FN(void,glGetShaderInfoLog,(unsigned,int,int*,char*));
 FN(void,glDeleteShader,(unsigned));
 FN(unsigned,glCreateProgram,(void));
 FN(void,glAttachShader,(unsigned,unsigned));
 FN(void,glLinkProgram,(unsigned));
 FN(void,glGetProgramiv,(unsigned,unsigned,int*));
 FN(void,glGetProgramInfoLog,(unsigned,int,int*,char*));
 FN(void,glUseProgram,(unsigned)); FN(void,glDeleteProgram,(unsigned));
 FN(void,glGenTextures,(int,unsigned*)); FN(void,glDeleteTextures,(int,const unsigned*));
 FN(void,glBindTexture,(unsigned,unsigned));
 FN(void,glTexImage2D,(unsigned,int,int,int,int,int,unsigned,unsigned,const void*));
 FN(void,glGenFramebuffers,(int,unsigned*)); FN(void,glDeleteFramebuffers,(int,const unsigned*));
 FN(void,glBindFramebuffer,(unsigned,unsigned));
 FN(void,glFramebufferTexture2D,(unsigned,unsigned,unsigned,unsigned,int));
 FN(unsigned,glCheckFramebufferStatus,(unsigned));
 FN(void,glBindBuffer,(unsigned,unsigned));
 FN(void,glPixelStorei,(unsigned,int));
 FN(void,glGenVertexArrays,(int,unsigned*)); FN(void,glBindVertexArray,(unsigned));
 FN(void,glDeleteVertexArrays,(int,const unsigned*));
 FN(void,glDrawArrays,(unsigned,int,int));
 FN(void,glReadPixels,(int,int,int,int,unsigned,unsigned,void*));
 const char *version=(const char*)glGetString(0x1f02);
 const char *vendor=(const char*)glGetString(0x1f00);
 int major=0;if(version)sscanf(version,"OpenGL ES %d",&major);
 fprintf(stderr,"ST-NATIVE-WARMUP: vendor=%s version=%s\n",vendor?vendor:"unknown",version?version:"unknown");
 if(major<3){fputs("ST-NATIVE-WARMUP-FAILED: GLES3 required\n",stderr);return 0;}
 /* Report and clear any capability-probe errors, with a bounded drain. */
 unsigned error=0;int i;
 for(i=0;i<16 && (error=glGetError())!=0;i++)fprintf(stderr,"ST native prior GL error: 0x%x\n",error);
 if(i==16)return 0;
 int old_program=0,old_tex=0,old_read=0,old_draw=0,old_pack=0,old_unpack=0,viewport[4]={0};
 int old_vao=0,pack[4]={0};
 const unsigned pack_keys[]={0x0d02,0x0d03,0x0d04,0x0d05};
 unsigned char mask[4]={0},enabled[6]={0};
 const unsigned caps[]={0x0be2,0x0b44,0x0b71,0x0c11,0x0b90,0x8c89}; /* blend,cull,depth,scissor,stencil,discard */
 glGetIntegerv(0x8b8d,&old_program);glGetIntegerv(0x8069,&old_tex);
 glGetIntegerv(0x8caa,&old_read);glGetIntegerv(0x8ca6,&old_draw);
 glGetIntegerv(0x88ed,&old_pack);glGetIntegerv(0x88ef,&old_unpack);
 glGetIntegerv(0x85b5,&old_vao);
 for(i=0;i<4;i++)glGetIntegerv(pack_keys[i],&pack[i]);
 glGetIntegerv(0x0ba2,viewport);glGetBooleanv(0x0c23,mask);
 for(i=0;i<6;i++){enabled[i]=glIsEnabled(caps[i]);glDisable(caps[i]);}
 unsigned shaders[2]={0},program=0,texture=0,fbo=0,vao=0;
 int ok=0,status=0;const char *stage="compile";char info[1024]={0};
 const char *sources[]={
  "#version 300 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1.,-1.):(gl_VertexID==1?vec2(3.,-1.):vec2(-1.,3.));gl_Position=vec4(p,0.,1.);}",
  "#version 300 es\nprecision mediump float;out vec4 color;void main(){color=vec4(.25,.5,.75,1.);}"};
 for(i=0;i<2;i++){
  shaders[i]=glCreateShader(i?0x8b30:0x8b31);if(!shaders[i])goto done;
  glShaderSource(shaders[i],1,&sources[i],NULL);glCompileShader(shaders[i]);
  glGetShaderiv(shaders[i],0x8b81,&status);
  if(!status){glGetShaderInfoLog(shaders[i],sizeof(info),NULL,info);goto done;}
 }
 stage="link";program=glCreateProgram();if(!program)goto done;
 for(i=0;i<2;i++)glAttachShader(program,shaders[i]);
 glLinkProgram(program);glGetProgramiv(program,0x8b82,&status);
 if(!status){glGetProgramInfoLog(program,sizeof(info),NULL,info);goto done;}
 stage="framebuffer";
 glBindBuffer(0x88eb,0);glBindBuffer(0x88ec,0);
 glGenTextures(1,&texture);glBindTexture(0x0de1,texture);
 glTexImage2D(0x0de1,0,0x8058,2,2,0,0x1908,0x1401,NULL);
 glGenFramebuffers(1,&fbo);glBindFramebuffer(0x8d40,fbo);
 glFramebufferTexture2D(0x8d40,0x8ce0,0x0de1,texture,0);
 if(glCheckFramebufferStatus(0x8d40)!=0x8cd5)goto done;
 stage="draw/readback";
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 for(i=0;i<4;i++)glPixelStorei(pack_keys[i],i==3?4:0);
 glUseProgram(program);glViewport(0,0,2,2);glColorMask(1,1,1,1);glDrawArrays(4,0,3);
 unsigned char pixel[4]={0};glReadPixels(0,0,1,1,0x1908,0x1401,pixel);
 error=glGetError();
 ok=!error && abs(pixel[0]-64)<=1 && abs(pixel[1]-128)<=1 && abs(pixel[2]-191)<=1 && pixel[3]==255;
 fprintf(stderr,"ST-NATIVE-WARMUP: pixel=%u,%u,%u,%u GL=0x%x\n",pixel[0],pixel[1],pixel[2],pixel[3],error);
 done:
 if(!ok)fprintf(stderr,"ST-NATIVE-WARMUP-FAILED: stage=%s GL=0x%x info=%s\n",stage,error?error:glGetError(),info);
 glUseProgram(old_program);glBindFramebuffer(0x8ca8,old_read);glBindFramebuffer(0x8ca9,old_draw);
 glBindTexture(0x0de1,old_tex);glBindBuffer(0x88eb,old_pack);glBindBuffer(0x88ec,old_unpack);
 glBindVertexArray(old_vao);
 for(i=0;i<4;i++)glPixelStorei(pack_keys[i],pack[i]);
 glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);
 for(i=0;i<6;i++)if(enabled[i])glEnable(caps[i]);
 if(vao)glDeleteVertexArrays(1,&vao);
 if(fbo)glDeleteFramebuffers(1,&fbo);
 if(texture)glDeleteTextures(1,&texture);
 if(program)glDeleteProgram(program);
 for(i=0;i<2;i++)if(shaders[i])glDeleteShader(shaders[i]);
 if(ok)fputs("ST-NATIVE-WARMUP-PASS: native shader, draw and readback before Wine mappings\n",stderr);
 return ok;
}
