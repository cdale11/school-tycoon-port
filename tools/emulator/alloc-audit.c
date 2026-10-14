/* SPDX-License-Identifier: MIT
 * VM diagnostic: observe calls to the executable's exported allocator.
 * LD_AUDIT reaches Box64's allocator even when it outranks LD_PRELOAD.
 * Each call still uses the exact original allocator and arguments.
 */
#define malloc st_trace_unused_malloc
#define calloc st_trace_unused_calloc
#define realloc st_trace_unused_realloc
#define free st_trace_unused_free
#define memalign st_trace_unused_memalign
#define aligned_alloc st_trace_unused_aligned_alloc
#define posix_memalign st_trace_unused_posix_memalign
#include "alloc-trace.c"
#include <link.h>
#include <string.h>

static void *(*original_malloc)(size_t);
static void *(*original_calloc)(size_t, size_t);
static void *(*original_realloc)(void *, size_t);
static void (*original_free)(void *);
static void *(*original_memalign)(size_t, size_t);
static void *(*original_aligned_alloc)(size_t, size_t);
static int (*original_posix_memalign)(void **, size_t, size_t);
#define ORIGINAL(name) __atomic_load_n(&original_##name, __ATOMIC_ACQUIRE)

static void *audit_malloc(size_t size) {
    void *p = ORIGINAL(malloc)(size);
    record(NULL, p, size, __builtin_return_address(0));
    return p;
}
static void *audit_calloc(size_t count, size_t size) {
    void *p = ORIGINAL(calloc)(count, size);
    if (!size || count <= SIZE_MAX / size)
        record(NULL, p, count * size, __builtin_return_address(0));
    return p;
}
static void *audit_realloc(void *old, size_t size) {
    void *p = ORIGINAL(realloc)(old, size);
    if (p || !size) record(old, p, size, __builtin_return_address(0));
    return p;
}
static void audit_free(void *p) {
    record(p, NULL, 0, NULL);
    ORIGINAL(free)(p);
}
static void *audit_memalign(size_t alignment, size_t size) {
    void *p = ORIGINAL(memalign)(alignment, size);
    record(NULL, p, size, __builtin_return_address(0));
    return p;
}
static void *audit_aligned_alloc(size_t alignment, size_t size) {
    void *p = ORIGINAL(aligned_alloc)(alignment, size);
    record(NULL, p, size, __builtin_return_address(0));
    return p;
}
static int audit_posix_memalign(void **result, size_t alignment, size_t size) {
    int status = ORIGINAL(posix_memalign)(result, alignment, size);
    if (!status) record(NULL, *result, size, __builtin_return_address(0));
    return status;
}
unsigned int la_version(unsigned int version) {
    return version < LAV_CURRENT ? version : LAV_CURRENT;
}
unsigned int la_objopen(struct link_map *map, Lmid_t namespace, uintptr_t *cookie) {
    (void)namespace;
    *cookie = !map->l_name[0]; /* Only allocations bound to the executable. */
    return LA_FLG_BINDTO | LA_FLG_BINDFROM;
}
uintptr_t la_symbind64(Elf64_Sym *symbol, unsigned int index,
                      uintptr_t *ref, uintptr_t *def, unsigned int *flags,
                      const char *name) {
    (void)index; (void)ref;
    *flags |= LA_SYMB_NOPLTENTER | LA_SYMB_NOPLTEXIT;
    if (*def) {
#define BIND(operation) \
        if (!strcmp(name, #operation)) { \
            __atomic_store_n(&original_##operation, (void *)symbol->st_value, __ATOMIC_RELEASE); \
            return (uintptr_t)audit_##operation; \
        }
        BIND(malloc) BIND(calloc) BIND(realloc) BIND(free)
        BIND(memalign) BIND(aligned_alloc) BIND(posix_memalign)
#undef BIND
    }
    return symbol->st_value;
}
