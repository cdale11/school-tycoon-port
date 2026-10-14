/* SPDX-License-Identifier: MIT
 * Keep a complete linked Wine vertex source available to GL4ES's internal
 * fixed-function variants, while exposing the application's original sources.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct shader_source { unsigned id; int length, modified; char *original, *backend; struct shader_source *next; };
struct linked_source { unsigned id, shader; char *joined; struct linked_source *next; };
static struct shader_source *shaders;
static struct linked_source *programs;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static void (*set_source)(unsigned,int,const char **,const int *);
static void (*get_source)(unsigned,int,int *,char *);
static void (*get_shader)(unsigned,unsigned,int *), (*get_program)(unsigned,unsigned,int *);
static void (*compile)(unsigned), (*use_program)(unsigned), (*delete_shader)(unsigned);
static unsigned char (*is_shader)(unsigned);

static void load(void) {
    const char *path=getenv("BOX64_LIBGL");
    void *h=path?dlopen(path,RTLD_NOW|RTLD_NOLOAD):NULL;
    if (!h) abort();
#define LOAD(v,n) v=dlsym(h,n); if(!v) abort()
    LOAD(set_source,"glShaderSource"); LOAD(get_source,"glGetShaderSource");
    LOAD(get_shader,"glGetShaderiv"); LOAD(get_program,"glGetProgramiv");
    LOAD(compile,"glCompileShader"); LOAD(use_program,"glUseProgram");
    LOAD(delete_shader,"glDeleteShader"); LOAD(is_shader,"glIsShader");
#undef LOAD
}
static struct shader_source *find_shader(unsigned id) {
    for(struct shader_source *s=shaders;s;s=s->next) if(s->id==id) return s;
    return NULL;
}
static void forget_program(unsigned id) {
    struct linked_source **p=&programs;
    while(*p && (*p)->id!=id) p=&(*p)->next;
    if(*p){struct linked_source *old=*p;*p=old->next;free(old->joined);free(old);}
}
static void restore(struct shader_source *s) {
    if(!s || !s->modified) return;
    const char *text=s->original;
    set_source(s->id,1,&text,NULL);
    free(s->backend);s->backend=NULL;s->modified=0;
}
void st_source_before_link(unsigned program, int count, const unsigned *attached) {
    pthread_once(&once,load);
    pthread_mutex_lock(&lock);
    forget_program(program);
    for(int i=0;i<count;i++) {
        struct shader_source *s=find_shader(attached[i]);
        if(s && s->modified) {restore(s);compile(s->id);}
    }
    pthread_mutex_unlock(&lock);
}
void st_source_joined(unsigned program,unsigned shader,const char *original,const char *joined) {
    pthread_once(&once,load);
    struct linked_source *p=calloc(1,sizeof(*p));
    char *text=strdup(joined);
    if(!p || !text){free(p);free(text);return;}
    pthread_mutex_lock(&lock);
    struct shader_source *s=find_shader(shader);
    if(!s) {
        s=calloc(1,sizeof(*s));
        if(s) {
            s->original=strdup(original);
            if(!s->original){free(s);s=NULL;}
            else {s->id=shader;s->length=(int)strlen(original)+1;s->next=shaders;shaders=s;}
        }
    }
    if(s){forget_program(program);p->id=program;p->shader=shader;p->joined=text;p->next=programs;programs=p;}
    else {free(text);free(p);}
    pthread_mutex_unlock(&lock);
}
void st_source_program_deleted(unsigned program) {
    pthread_mutex_lock(&lock);forget_program(program);pthread_mutex_unlock(&lock);
}
void st_glUseProgram(unsigned program) {
    pthread_once(&once,load);
    pthread_mutex_lock(&lock);
    for(struct linked_source *p=programs;p;p=p->next) if(p->id==program) {
        int linked=0;get_program(program,0x8b82,&linked);
        struct shader_source *s=linked?find_shader(p->shader):NULL;
        if(s && (!s->modified || strcmp(s->backend,p->joined))) {
            char *copy=strdup(p->joined);
            if(copy){const char *text=copy;set_source(s->id,1,&text,NULL);free(s->backend);s->backend=copy;s->modified=1;}
        }
        break;
    }
    pthread_mutex_unlock(&lock);
    use_program(program);
}
void st_glShaderSource(unsigned shader,int count,const char **strings,const int *lengths) {
    pthread_once(&once,load);
    pthread_mutex_lock(&lock);
    struct shader_source *s=find_shader(shader);
    if(s && count>=0 && is_shader(shader)) restore(s);
    set_source(shader,count,strings,lengths);
    if(s && count>=0 && is_shader(shader)) {
        int length=0;get_shader(shader,0x8b88,&length);
        if(length>=0 && length<=1048576) {
            char *copy=calloc(1,(size_t)length+1);
            if(copy){if(length)get_source(shader,length,NULL,copy);free(s->original);s->original=copy;s->length=length;}
        }
    }
    pthread_mutex_unlock(&lock);
}
void st_glCompileShader(unsigned shader) {
    pthread_once(&once,load);
    pthread_mutex_lock(&lock);restore(find_shader(shader));pthread_mutex_unlock(&lock);
    compile(shader);
}
void st_glGetShaderiv(unsigned shader,unsigned pname,int *value) {
    pthread_once(&once,load);
    pthread_mutex_lock(&lock);
    struct shader_source *s=find_shader(shader);
    if(s && pname==0x8b88){*value=s->length;pthread_mutex_unlock(&lock);return;}
    pthread_mutex_unlock(&lock);get_shader(shader,pname,value);
}
void st_glGetShaderSource(unsigned shader,int size,int *length,char *output) {
    pthread_once(&once,load);
    pthread_mutex_lock(&lock);
    struct shader_source *s=find_shader(shader);
    if(s && size>0 && output) {
        size_t n=strlen(s->original);if(n>=(size_t)size)n=(size_t)size-1;
        memcpy(output,s->original,n);output[n]=0;if(length)*length=(int)n;
        pthread_mutex_unlock(&lock);return;
    }
    pthread_mutex_unlock(&lock);get_source(shader,size,length,output);
}
void st_glDeleteShader(unsigned shader) {
    pthread_once(&once,load);delete_shader(shader);
    if(is_shader(shader)) return; /* Delete-pending attached objects remain queryable. */
    pthread_mutex_lock(&lock);
    struct shader_source **p=&shaders;
    while(*p && (*p)->id!=shader)p=&(*p)->next;
    if(*p){struct shader_source *old=*p;*p=old->next;free(old->original);free(old->backend);free(old);}
    pthread_mutex_unlock(&lock);
}
