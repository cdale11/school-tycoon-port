/* SPDX-License-Identifier: MIT
 * Exercise joined-source internal alpha-test variants through real GL4ES/GLES.
 * Run in the prepared ARM64 VM with the presentation helper preloaded.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOAD(handle, result, name, parameters) \
    result (*name) parameters = dlsym(handle, #name); if (!name) return 2

int main(void) {
    void *desktop = dlopen(getenv("BOX64_LIBGL"), RTLD_NOW | RTLD_GLOBAL);
    if (!desktop) return 1;
    Display *display = XOpenDisplay(NULL);
    if (!display) return 1;
    LOAD(desktop, XVisualInfo *, glXChooseVisual, (Display *, int, int *));
    LOAD(desktop, void *, glXCreateContext, (Display *, XVisualInfo *, void *, int));
    LOAD(desktop, int, glXMakeCurrent, (Display *, unsigned long, void *));
    LOAD(desktop, void *, glXGetProcAddressARB, (const unsigned char *));
    XVisualInfo *visual = glXChooseVisual(display, DefaultScreen(display), NULL);
    if (!visual) return 2;
    void *context = glXCreateContext(display, visual, NULL, 1);
    if (!context || !glXMakeCurrent(display, DefaultRootWindow(display), context)) return 2;

    LOAD(desktop, void, glGenTextures, (int, unsigned *));
    LOAD(desktop, void, glBindTexture, (unsigned, unsigned));
    LOAD(desktop, void, glTexImage2D, (unsigned, int, int, int, int, int, unsigned, unsigned, const void *));
    LOAD(desktop, void, glGenFramebuffers, (int, unsigned *));
    LOAD(desktop, void, glBindFramebuffer, (unsigned, unsigned));
    LOAD(desktop, void, glFramebufferTexture2D, (unsigned, unsigned, unsigned, unsigned, int));
    LOAD(desktop, void, glClearColor, (float, float, float, float));
    LOAD(desktop, void, glClear, (unsigned));
    unsigned texture = 0, framebuffer = 0;
    glGenTextures(1, &texture);
    glBindTexture(0x0de1, texture);
    glTexImage2D(0x0de1, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, NULL);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(0x8d40, framebuffer);
    glFramebufferTexture2D(0x8d40, 0x8ce0, 0x0de1, texture, 0);
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(0x4000);

    LOAD(desktop, unsigned, glCreateShader, (unsigned));
    LOAD(desktop, void, glShaderSource, (unsigned,int,const char**,const int*));
    LOAD(desktop, void, glCompileShader, (unsigned));
    LOAD(desktop, unsigned, glCreateProgram, (void));
    LOAD(desktop, void, glAttachShader, (unsigned,unsigned));
    LOAD(desktop, void, glGetProgramiv, (unsigned,unsigned,int*));
    LOAD(desktop, void, glUseProgram, (unsigned));
    LOAD(desktop, void, glViewport, (int,int,int,int));
    LOAD(desktop, void, glBegin, (unsigned));
    LOAD(desktop, void, glVertex2f, (float,float));
    LOAD(desktop, void, glEnd, (void));
    LOAD(desktop, void, glReadPixels, (int,int,int,int,unsigned,unsigned,void*));
    const char *vs = "#version 120\nuniform mat4 matrices[8]; uniform vec4 colours[8];\nvoid setup_vs_output(in vec4[2]);\n"
        "void main(){vec4 v[2];v[0]=matrices[0]*gl_Vertex;v[1]=colours[0];setup_vs_output(v);}\n";
    const char *out = "#version 120\ninvariant gl_Position;\n"
        "void setup_vs_output(in vec4 outputs[2]){gl_Position=outputs[0];gl_TexCoord[0]=outputs[1];}\n";
    const char *ps = "#version 120\nuniform vec4 scalar; void main(){gl_FragColor=gl_TexCoord[0]+scalar;}\n";
    unsigned shaders[3] = {glCreateShader(0x8b31), glCreateShader(0x8b31), glCreateShader(0x8b30)};
    const char *texts[3] = {vs, out, ps};
    unsigned program = glCreateProgram();
    for (int i = 0; i < 3; i++) {
        glShaderSource(shaders[i], 1, &texts[i], NULL);
        glCompileShader(shaders[i]);
        glAttachShader(program, shaders[i]);
    }
    void (*link)(unsigned) = glXGetProcAddressARB((const unsigned char *)"glLinkProgram");
    if (!link) return 2;
    LOAD(desktop, void, glDeleteShader, (unsigned));
    glDeleteShader(shaders[1]); /* Wine deletes its attached output object. */
    int linked = 0;
    for (int iteration = 0; iteration < 64; iteration++) {
        link(program);
        glGetProgramiv(program, 0x8b82, &linked);
        if (!linked) return 4;
        int count = 0;
        glGetProgramiv(program, 0x8b85, &count);
        if (count != 3) return 5;
    }
    printf("Joined vertex shader link=%d\n", linked);
    void (*use)(unsigned)=glXGetProcAddressARB((const unsigned char*)"glUseProgram");
    if(!use)return 2;
    use(program);
    void (*source_query)(unsigned,int,int*,char*)=glXGetProcAddressARB((const unsigned char*)"glGetShaderSource");
    void (*shader_query)(unsigned,unsigned,int*)=glXGetProcAddressARB((const unsigned char*)"glGetShaderiv");
    char original[4096]={0};int source_length=0;
    if(!source_query||!shader_query)return 2;
    source_query(shaders[0],sizeof(original),NULL,original);shader_query(shaders[0],0x8b88,&source_length);
    if(strcmp(original,vs)||source_length!=(int)strlen(vs)+1)return 11;
    int (*location)(unsigned,const char*)=glXGetProcAddressARB((const unsigned char*)"glGetUniformLocation");
    void (*vector)(int,int,const float*)=glXGetProcAddressARB((const unsigned char*)"glUniform4fv");
    void (*matrix)(int,int,unsigned char,const float*)=glXGetProcAddressARB((const unsigned char*)"glUniformMatrix4fv");
    LOAD(desktop, unsigned, glGetError, (void));
    if(!location||!vector||!matrix)return 2;
    int bad=location(program,"matrices[1]");
    printf("Inactive array location=%d\n",bad);if(bad!=-1)return 7;
    float matrices[16][16]={{0}},colours[16][4]={{0}};
    for(int i=0;i<16;i++)for(int j=0;j<4;j++)matrices[i][j*5]=1.0f;
    colours[0][0]=0.8f;colours[0][1]=0.2f;colours[0][2]=0.1f;colours[0][3]=0.25f;
    int scalar=location(program,"scalar");if(scalar<0)return 8;
    vector(scalar,2,&colours[0][0]);
    unsigned error=glGetError();printf("Scalar oversize error=%x\n",error);if(error!=0x502)return 9;
    vector(location(program,"colours[0]"),16,&colours[0][0]);
    matrix(location(program,"matrices[0]"),16,0,&matrices[0][0]);
    error=glGetError();printf("Array oversize error=%x\n",error);if(error)return 10;
    LOAD(desktop, void, glEnable, (unsigned));
    LOAD(desktop, void, glAlphaFunc, (unsigned,float));
    glEnable(0x0bc0); glAlphaFunc(0x0204,0.5f);
    glViewport(0, 0, 16, 16);
    glBegin(7);
    glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, 1); glVertex2f(-1, 1);
    glEnd();
    unsigned char pixel[4] = {0};
    glReadPixels(8, 8, 1, 1, 0x1908, 0x1401, pixel);
    printf("Joined shader pixel=%u %u %u %u\n", pixel[0], pixel[1], pixel[2], pixel[3]);
    if (abs(pixel[0] - 64) > 2 || abs(pixel[1] - 128) > 2 || abs(pixel[2] - 191) > 2) return 6;
    for (int iteration = 0; iteration < 16; iteration++) {
        link(program);
        glGetProgramiv(program, 0x8b82, &linked);
        if (!linked) return 12;
        use(program);
        vector(location(program,"colours[0]"),16,&colours[0][0]);
        matrix(location(program,"matrices[0]"),16,0,&matrices[0][0]);
        glClear(0x4000);
        glBegin(7);
        glVertex2f(-1,-1); glVertex2f(1,-1); glVertex2f(1,1); glVertex2f(-1,1);
        glEnd();
        glReadPixels(8,8,1,1,0x1908,0x1401,pixel);
        if (abs(pixel[0]-64)>2 || abs(pixel[1]-128)>2 || abs(pixel[2]-191)>2) return 13;
        source_query(shaders[0],sizeof(original),NULL,original);
        if (strcmp(original,vs)) return 14;
    }
    puts("ST-FPE-PASS: joined source, original-source queries, array writes, alpha-test discard");
    return 0;
}
