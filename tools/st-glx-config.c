/* SPDX-License-Identifier: MIT
 * PortMaster Weston 0.2 Crusty embeds GLXFBConfig data in its result array.
 * GLX callers own that array, but configuration handles must remain valid.
 * Return a separate caller-owned array and retain the backing allocation.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct retained { void *allocation; struct retained *next; };
static struct retained *allocations;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *bridge_symbol(const char *name) {
    const char *path = getenv("ST_CRUSTY_LIBRARY");
    if (!path || !*path) path = "libcrusty.so";
    /* Box64's resolver can return this interposer for RTLD_NEXT. Resolve
     * through the already-loaded bridge's own handle instead. */
    void *handle = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
    if (!handle) return NULL;
    void *symbol = dlsym(handle, name);
    dlclose(handle);
    return symbol;
}

static void **retain_configs(void **configs, int count) {
    if (!configs || count <= 0) return configs;
    void **result = malloc((size_t)count * sizeof(*result));
    struct retained *entry = malloc(sizeof(*entry));
    if (!result || !entry) {
        free(result); free(entry);
        fputs("ST graphics: cannot retain GLX configurations\n", stderr);
        abort();
    }
    memcpy(result, configs, (size_t)count * sizeof(*result));
    entry->allocation = configs;
    pthread_mutex_lock(&lock);
    int first = allocations == NULL;
    entry->next = allocations; allocations = entry;
    pthread_mutex_unlock(&lock);
    if (first) fputs("ST graphics: retained GLX framebuffer configurations\n", stderr);
    return result;
}

void **crusty_glXGetFBConfigs(void *display, int screen, int *count) {
    void **(*next)(void *, int, int *) = bridge_symbol("crusty_glXGetFBConfigs");
    if (!next || next == crusty_glXGetFBConfigs) { if (count) *count = 0; return NULL; }
    void **configs = next(display, screen, count);
    return retain_configs(configs, *count);
}

void **crusty_glXChooseFBConfig(void *display, int screen, const int *attributes, int *count) {
    void **(*next)(void *, int, const int *, int *) = bridge_symbol("crusty_glXChooseFBConfig");
    if (!next || next == crusty_glXChooseFBConfig) { if (count) *count = 0; return NULL; }
    void **configs = next(display, screen, attributes, count);
    return retain_configs(configs, *count);
}

void **glXGetFBConfigs(void *display, int screen, int *count) {
    return crusty_glXGetFBConfigs(display, screen, count);
}
void **glXChooseFBConfig(void *display, int screen, const int *attributes, int *count) {
    return crusty_glXChooseFBConfig(display, screen, attributes, count);
}

/* Configurations are immutable; retain them until this process exits. */
__attribute__((destructor)) static void release_configs(void) {
    while (allocations) {
        struct retained *entry = allocations;
        allocations = entry->next;
        free(entry->allocation); free(entry);
    }
}
