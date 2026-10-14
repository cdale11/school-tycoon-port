"""Shader-cache deletion/ID-reuse and shared-shader regression against native API fixture."""
from pathlib import Path
import subprocess,tempfile,os
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-source-') as d:
 root=Path(d)
 (root/'backend.c').write_text(r'''
#include <string.h>
static int valid, pending, refs;
static char source[1024];
unsigned glCreateShader(unsigned type){(void)type;valid=1;pending=refs=0;source[0]=0;return 1;}
unsigned char glIsShader(unsigned id){return id==1&&valid;}
void glDeleteShader(unsigned id){(void)id;pending=1;if(!refs)valid=0;}
void glShaderSource(unsigned id,int count,const char **text,const int *lengths){(void)id;(void)lengths;if(count)strcpy(source,text[0]);}
void glGetShaderSource(unsigned id,int size,int *length,char *out){(void)id;int n=strlen(source);if(n>=size)n=size-1;if(size>0){memcpy(out,source,n);out[n]=0;}if(length)*length=n;}
void glGetShaderiv(unsigned id,unsigned pname,int *out){(void)id;(void)pname;*out=strlen(source)+1;}
void glGetProgramiv(unsigned id,unsigned pname,int *out){(void)id;(void)pname;*out=1;}
void glCompileShader(unsigned id){(void)id;}
void glUseProgram(unsigned id){(void)id;}
void mock_attach(void){refs++;}
void mock_delete_program(void){if(!--refs&&pending)valid=0;}
const char *mock_source(void){return source;}
''')
 driver=r'''
#include <assert.h>
#include "SOURCE_FILE"
int main(int argc,char **argv){
 assert(argc==2);setenv("BOX64_LIBGL",argv[1],1);
 void *h=dlopen(argv[1],RTLD_NOW|RTLD_GLOBAL);assert(h);
 void (*attach)(void)=dlsym(h,"mock_attach");
 void (*destroy)(void)=dlsym(h,"mock_delete_program");
 const char *(*backend)(void)=dlsym(h,"mock_source");
 for(int i=0;i<1000;i++){
  unsigned id=st_glCreateShader(0x8b31);assert(id==1);attach();attach();
  st_source_joined(10,id,"original","joined A");st_source_joined(20,id,"original","joined B");
  st_glUseProgram(10);assert(!strcmp(backend(),"joined A"));
  st_glUseProgram(20);assert(!strcmp(backend(),"joined B"));
  st_glUseProgram(10);assert(!strcmp(backend(),"joined A"));
  char out[64];st_glGetShaderSource(id,64,0,out);assert(!strcmp(out,"original"));
  st_glDeleteShader(id);assert(shaders);destroy();st_source_program_deleted(20);assert(shaders);
  destroy();st_source_program_deleted(10);assert(!shaders&&!programs);
 }
 // A last detach can destroy a pending shader without a cached-program callback.
 unsigned id=st_glCreateShader(0x8b31);attach();st_source_joined(30,id,"old","joined old");
 st_glDeleteShader(id);destroy();assert(shaders);
 id=st_glCreateShader(0x8b31);assert(!shaders&&!programs);
 const char *text="new shader";st_glShaderSource(id,1,&text,0);
 char out[64];st_glGetShaderSource(id,64,0,out);assert(!strcmp(out,"new shader"));
 return 0;
}
'''.replace('SOURCE_FILE',str(repo/'tools/st-gl-source.c'))
 (root/'driver.c').write_text(driver)
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-fPIC','-shared',str(root/'backend.c'),'-o',str(root/'backend.so')],check=True)
 subprocess.run(['cc','-Wall','-Wextra','-Werror',str(root/'driver.c'),'-ldl','-pthread','-o',str(root/'driver')],check=True)
 subprocess.run([str(root/'driver'),str(root/'backend.so')],check=True,timeout=10)
 print('PASS: 1000 pending-deletion cycles, zero retained entries, ID reuse, shared-program source switches')
