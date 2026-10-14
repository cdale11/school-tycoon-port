"""Exercise opaque/non-contiguous locations and inactive array element zero."""
from pathlib import Path
import subprocess,tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-uniform-') as d:
 root=Path(d)
 (root/'backend.c').write_text(r'''
#include <string.h>
int observed;
void glGetIntegerv(unsigned name,int *value){(void)name;*value=42;}
int glGetUniformLocation(unsigned id,const char *name){(void)id;if(!strcmp(name,"values[1]"))return 17;if(!strcmp(name,"values[2]"))return 99;return -1;}
void glGetProgramiv(unsigned id,unsigned name,int *out){(void)id;*out=name==0x8b87?32:1;}
void glGetActiveUniform(unsigned id,unsigned index,int size,int *length,int *count,unsigned *type,char *name){(void)id;(void)index;(void)size;strcpy(name,"values[0]");*length=9;*count=3;*type=0x8b52;}
void glUniform4fv(int location,int count,const float *values){(void)location;(void)values;observed=count;}
void glUniformMatrix4fv(int location,int count,unsigned char transpose,const float *values){(void)location;(void)transpose;(void)values;observed=count;}
void glDeleteProgram(unsigned id){(void)id;}
unsigned char glIsProgram(unsigned id){return id==42;}
''')
 (root/'driver.c').write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include <dlfcn.h>
void st_uniform_linked(unsigned);void st_glUniform4fv(int,int,const float*);
void st_glUniformMatrix4fv(int,int,unsigned char,const float*);void st_glDeleteProgram(unsigned);
static int removed;
void st_source_program_deleted(unsigned id){assert(id==42);removed++;}
int main(int argc,char **argv){assert(argc==2);setenv("BOX64_LIBGL",argv[1],1);setenv("SDL_VIDEO_GL_DRIVER",argv[1],1);
void *h=dlopen(argv[1],RTLD_NOW|RTLD_GLOBAL);assert(h);int *observed=dlsym(h,"observed");
float values[128]={0};st_uniform_linked(42);
st_glUniform4fv(17,99,values);assert(*observed==2);
st_glUniformMatrix4fv(99,55,0,values);assert(*observed==1);
st_glUniform4fv(123,2,values);assert(*observed==2); // Leave scalar oversize to backend error handling.
st_glDeleteProgram(999);assert(!removed);st_glDeleteProgram(42);assert(removed==1);return 0;}
''')
 subprocess.run(['cc','-shared','-fPIC',str(root/'backend.c'),'-o',str(root/'backend.so')],check=True)
 subprocess.run(['cc','-Wall','-Wextra','-Werror',str(root/'driver.c'),str(repo/'tools/st-gl-uniform.c'),'-ldl','-pthread','-o',str(root/'driver')],check=True)
 subprocess.run([str(root/'driver'),str(root/'backend.so')],check=True,timeout=10)
 print('PASS: inactive base, non-contiguous locations, remaining vector/matrix bounds, scalar validation, invalid deletion')
