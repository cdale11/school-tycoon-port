/* SPDX-License-Identifier: MIT
 * Preserve desktop GL array semantics in the pinned GL4ES/GLES bridge.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct array { int base, size; struct array *next; };
struct program { unsigned id; struct array *arrays; struct program *next; };
static struct program *programs;
static pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t symbols_once = PTHREAD_ONCE_INIT;
static void (*desktop_get)(unsigned, int *);
static int (*desktop_location)(unsigned, const char *), (*native_location)(unsigned, const char *);
static void (*native_program)(unsigned, unsigned, int *);
static void (*native_uniform)(unsigned, unsigned, int, int *, int *, unsigned *, char *);
static void (*desktop_vector)(int, int, const float *);
static void (*desktop_matrix)(int, int, unsigned char, const float *);
static void (*desktop_delete)(unsigned);
static int available;

static void load(void) {
    const char *p = getenv("BOX64_LIBGL"), *n = getenv("SDL_VIDEO_GL_DRIVER");
    void *desktop = p ? dlopen(p, RTLD_NOW | RTLD_NOLOAD) : NULL;
    void *native = n ? dlopen(n, RTLD_NOW | RTLD_NOLOAD) : NULL;
    if (!desktop || !native) return;
#define LOAD(h, v, name) v = dlsym(h, name); if (!v) return
    LOAD(desktop, desktop_get, "glGetIntegerv");
    LOAD(desktop, desktop_location, "glGetUniformLocation");
    LOAD(desktop, desktop_vector, "glUniform4fv");
    LOAD(desktop, desktop_matrix, "glUniformMatrix4fv");
    LOAD(desktop, desktop_delete, "glDeleteProgram");
    LOAD(native, native_location, "glGetUniformLocation");
    LOAD(native, native_program, "glGetProgramiv");
    LOAD(native, native_uniform, "glGetActiveUniform");
#undef LOAD
    available = 1;
}

static void forget(unsigned id) {
    struct program **p = &programs;
    while (*p && (*p)->id != id) p = &(*p)->next;
    if (!*p) return;
    struct program *old = *p;
    *p = old->next;
    while (old->arrays) {
        struct array *a = old->arrays;
        old->arrays = a->next;
        free(a);
    }
    free(old);
}

void st_uniform_linked(unsigned id) {
    pthread_once(&symbols_once, load);
    if (!available) return;
    pthread_mutex_lock(&cache_lock);
    forget(id);
    int linked = 0, count = 0, max_name = 0;
    native_program(id, 0x8b82, &linked);
    if (!linked) { pthread_mutex_unlock(&cache_lock); return; }
    native_program(id, 0x8b86, &count);
    native_program(id, 0x8b87, &max_name);
    if (max_name <= 0 || max_name > 1048576) { pthread_mutex_unlock(&cache_lock); return; }
    char *name = malloc((size_t)max_name);
    struct program *p = calloc(1, sizeof(*p));
    if (!name || !p) { free(name); free(p); pthread_mutex_unlock(&cache_lock); return; }
    p->id = id; p->next = programs; programs = p;
    for (int i = 0; i < count; i++) {
        int length = 0, size = 0; unsigned type = 0;
        native_uniform(id, (unsigned)i, max_name, &length, &size, &type, name);
        if (length <= 0 || name[length - 1] != ']' || size <= 0) continue;
        int base = native_location(id, name);
        if (base < 0) continue;
        struct array *a = malloc(sizeof(*a));
        if (!a) continue;
        a->base = base; a->size = size; a->next = p->arrays; p->arrays = a;
    }
    free(name);
    pthread_mutex_unlock(&cache_lock);
}

int st_glGetUniformLocation(unsigned program, const char *name) {
    pthread_once(&symbols_once, load);
    if (!desktop_location) abort();
    int location = desktop_location(program, name);
    if (available && location >= 0 && strchr(name, '['))
        return native_location(program, name);
    return location;
}

static int array_count(int location, int count) {
    if (!available || location < 0 || count <= 1) return count;
    int id = 0;
    desktop_get(0x8b8d, &id); /* GL_CURRENT_PROGRAM */
    pthread_mutex_lock(&cache_lock);
    for (struct program *p = programs; p; p = p->next) if (p->id == (unsigned)id) {
        for (struct array *a = p->arrays; a; a = a->next)
            if (location >= a->base && location - a->base < a->size) {
                int remaining = a->size - (location - a->base);
                if (count > remaining) count = remaining;
                break;
            }
        break;
    }
    pthread_mutex_unlock(&cache_lock);
    return count;
}

void st_glUniform4fv(int location, int count, const float *values) {
    pthread_once(&symbols_once, load);
    if (!desktop_vector) abort();
    desktop_vector(location, array_count(location, count), values);
}

void st_glUniformMatrix4fv(int location, int count, unsigned char transpose, const float *values) {
    pthread_once(&symbols_once, load);
    if (!desktop_matrix) abort();
    desktop_matrix(location, array_count(location, count), transpose, values);
}

void st_glDeleteProgram(unsigned id) {
    pthread_once(&symbols_once, load);
    if (!desktop_delete) abort();
    desktop_delete(id);
    pthread_mutex_lock(&cache_lock); forget(id); pthread_mutex_unlock(&cache_lock);
}
