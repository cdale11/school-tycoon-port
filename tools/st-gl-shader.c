/* SPDX-License-Identifier: MIT
 * Join Wine's separate setup_vs_output definition for the pinned GL4ES bridge.
 * Its GL shader/program handles are native GLES handles. Change only native
 * attachments while linking; keep GL4ES's shader ownership and sources intact.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_once_t symbols_once = PTHREAD_ONCE_INIT;
static void (*get_program)(unsigned, unsigned, int *);
static void (*get_shader)(unsigned, unsigned, int *);
static void (*get_attached)(unsigned, int, int *, unsigned *);
static void (*get_source)(unsigned, int, int *, char *);
static void (*set_source)(unsigned, int, const char **, const int *);
static void (*compile_shader)(unsigned), (*delete_shader)(unsigned);
static void (*link_program)(unsigned);
static unsigned (*create_shader)(unsigned);
static void (*native_attach)(unsigned, unsigned), (*native_detach)(unsigned, unsigned);
static void (*native_delete_program)(unsigned);
static unsigned (*native_create_program)(void);
static int available;
static pthread_mutex_t report_lock = PTHREAD_MUTEX_INITIALIZER;
static int reported;
void st_source_before_link(unsigned, int, const unsigned *);
void st_source_joined(unsigned, unsigned, const char *, const char *);

static void resolve_symbols(void) {
    const char *desktop_path = getenv("BOX64_LIBGL");
    const char *native_path = getenv("SDL_VIDEO_GL_DRIVER");
    void *desktop = desktop_path ? dlopen(desktop_path, RTLD_NOW | RTLD_NOLOAD) : NULL;
    void *native = native_path ? dlopen(native_path, RTLD_NOW | RTLD_NOLOAD) : NULL;
    if (!desktop) return;
    link_program = dlsym(desktop, "glLinkProgram");
    if (!native) return;
#define LOAD(handle, target, name) target = dlsym(handle, name); if (!target) return
    LOAD(desktop, get_program, "glGetProgramiv");
    LOAD(desktop, get_shader, "glGetShaderiv");
    LOAD(desktop, get_attached, "glGetAttachedShaders");
    LOAD(desktop, get_source, "glGetShaderSource");
    LOAD(desktop, set_source, "glShaderSource");
    LOAD(desktop, compile_shader, "glCompileShader");
    LOAD(desktop, delete_shader, "glDeleteShader");
    LOAD(desktop, create_shader, "glCreateShader");
    LOAD(native, native_attach, "glAttachShader");
    LOAD(native, native_detach, "glDetachShader");
    LOAD(native, native_create_program, "glCreateProgram");
    LOAD(native, native_delete_program, "glDeleteProgram");
#undef LOAD
    available = link_program != NULL;
    /* Retain handles for the graphics session. */
}

void st_glLinkProgram(unsigned program) {
    pthread_once(&symbols_once, resolve_symbols);
    if (!link_program) {
        fputs("ST graphics: shader linker unavailable\n", stderr);
        abort();
    }
    int count = 0;
    if (!available) { link_program(program); return; }
    get_program(program, 0x8b85, &count); /* GL_ATTACHED_SHADERS */
    if (count < 2 || count > 8) { link_program(program); return; }
    unsigned shaders[8], main_id = 0, output_id = 0;
    char *main_source = NULL, *output_source = NULL;
    int vertices = 0;
    get_attached(program, 8, &count, shaders);
    st_source_before_link(program, count, shaders);
    for (int i = 0; i < count; i++) {
        int type = 0, length = 0;
        get_shader(shaders[i], 0x8b4f, &type); /* GL_SHADER_TYPE */
        if (type != 0x8b31) continue;          /* GL_VERTEX_SHADER */
        vertices++;
        get_shader(shaders[i], 0x8b88, &length); /* GL_SHADER_SOURCE_LENGTH */
        if (length <= 0 || length > 1048576) continue;
        char *source = calloc(1, (size_t)length + 1);
        if (!source) continue;
        get_source(shaders[i], length, NULL, source);
        if (strstr(source, "void main(") && strstr(source, "void setup_vs_output(")) {
            free(main_source); main_source = source; main_id = shaders[i];
        } else if (!strstr(source, "void main(") && strstr(source, "void setup_vs_output(")
                   && !strstr(source, "uniform ")) {
            /* Point-size variants with extra uniforms require separate support. */
            free(output_source); output_source = source; output_id = shaders[i];
        } else free(source);
    }
    if (vertices != 2 || !main_source || !output_source) goto fallback;
    const char *function = strstr(output_source, "void setup_vs_output(");
    if (!strchr(function, '{')) goto fallback;
    size_t main_length = strlen(main_source), function_length = strlen(function);
    char *joined = malloc(main_length + function_length + 2);
    if (!joined) goto fallback;
    memcpy(joined, main_source, main_length);
    joined[main_length] = '\n';
    memcpy(joined + main_length + 1, function, function_length + 1);
    unsigned combined = create_shader(0x8b31);
    if (!combined) { free(joined); goto fallback; }
    const char *text = joined;
    set_source(combined, 1, &text, NULL);
    compile_shader(combined);
    int compiled = 0;
    get_shader(combined, 0x8b81, &compiled); /* GL_COMPILE_STATUS */
    unsigned holding = compiled ? native_create_program() : 0;
    if (!holding) { free(joined); delete_shader(combined); goto fallback; }

    /* Keep delete-pending original shaders alive while temporarily detached. */
    native_attach(holding, main_id); native_attach(holding, output_id);
    native_detach(program, main_id); native_detach(program, output_id);
    native_attach(program, combined);
    link_program(program);
    native_detach(program, combined);
    native_attach(program, main_id); native_attach(program, output_id);
    native_delete_program(holding);
    delete_shader(combined);
    int linked = 0;
    get_program(program, 0x8b82, &linked);
    if (linked) st_source_joined(program, main_id, main_source, joined);
    free(joined);
    pthread_mutex_lock(&report_lock);
    if (!reported) {
        fputs("ST graphics: joined Wine vertex output function\n", stderr);
        reported = 1;
    }
    pthread_mutex_unlock(&report_lock);
    free(main_source); free(output_source);
    return;
fallback:
    free(main_source); free(output_source);
    link_program(program);
}
