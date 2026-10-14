/* SPDX-License-Identifier: MIT
 * Private-VM diagnostic: attribute live large native glibc allocations.
 * This is instrumentation, not a handheld runtime component.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void *__libc_malloc(size_t);
extern void *__libc_calloc(size_t, size_t);
extern void *__libc_realloc(void *, size_t);
extern void __libc_free(void *);

#define SLOTS 262144
#define SITES 256
#define MIN_SIZE 131072
static size_t minimum_size = MIN_SIZE;
static unsigned dump_interval = 100;
struct allocation { void *pointer; size_t size; unsigned site; };
struct site { void *caller; size_t live, peak, total; unsigned count; };
static struct allocation allocations[SLOTS];
static struct site sites[SITES];
static unsigned site_count, events, dropped;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static __thread int inside __attribute__((tls_model("initial-exec")));
static pthread_once_t aligned_once = PTHREAD_ONCE_INIT;
static int (*real_posix_memalign)(void **, size_t, size_t);
static void *(*real_aligned_alloc)(size_t, size_t), *(*real_memalign)(size_t, size_t);

static void aligned_symbols(void) {
    real_posix_memalign = dlsym(RTLD_NEXT, "posix_memalign");
    real_aligned_alloc = dlsym(RTLD_NEXT, "aligned_alloc");
    real_memalign = dlsym(RTLD_NEXT, "memalign");
}
static void resolve_aligned(void) {
    int previous = inside;
    inside = 1;
    pthread_once(&aligned_once, aligned_symbols);
    inside = previous;
}
__attribute__((constructor)) static void configure(void) {
    const char *value = getenv("ST_ALLOCATION_MIN_SIZE");
    char *end;
    if (value && *value) {
        unsigned long size = strtoul(value, &end, 10);
        if (!*end && size >= 256 && size <= 67108864) minimum_size = size;
    }
    value = getenv("ST_ALLOCATION_DUMP_INTERVAL");
    if (value && *value) {
        unsigned long interval = strtoul(value, &end, 10);
        if (!*end && interval && interval <= 1000000) dump_interval = (unsigned)interval;
    }
}

static unsigned find(void *p) {
    unsigned h = ((uintptr_t)p >> 4) % SLOTS;
    for (unsigned n = 0; n < SLOTS; n++, h = (h + 1) % SLOTS)
        if (!allocations[h].pointer || allocations[h].pointer == p) return h;
    return SLOTS;
}

static void remove_allocation(void *p) {
    if (!p) return;
    unsigned h = find(p);
    if (h == SLOTS || allocations[h].pointer != p) return;
    sites[allocations[h].site].live -= allocations[h].size;
    allocations[h].pointer = (void *)1; /* Tombstone preserves collision chains. */
}

static void dump(void) {
    char line[512];
    int n = snprintf(line, sizeof(line), "ST allocations pid=%ld events=%u dropped=%u minimum=%zu interval=%u\n",
                     (long)getpid(), events, dropped, minimum_size, dump_interval);
    write(2, line, (size_t)n);
    for (unsigned i = 0; i < site_count; i++) {
        Dl_info info = {0};
        dladdr(sites[i].caller, &info);
        n = snprintf(line, sizeof(line),
            "ST alloc pid=%ld site=%u live=%zu peak=%zu total=%zu count=%u caller=%p %s+0x%lx\n",
            (long)getpid(), i, sites[i].live, sites[i].peak, sites[i].total, sites[i].count,
            sites[i].caller, info.dli_fname ? info.dli_fname : "?",
            (unsigned long)((uintptr_t)sites[i].caller - (uintptr_t)info.dli_fbase));
        if (n > 0) write(2, line, (size_t)n < sizeof(line) ? (size_t)n : sizeof(line)-1);
    }
}

static void record(void *old, void *p, size_t size, void *caller) {
    if (inside || (!old && (!p || size < minimum_size))) return;
    int saved_errno = errno;
    inside = 1;
    pthread_mutex_lock(&lock);
    remove_allocation(old);
    if (p && size >= minimum_size) {
        unsigned i;
        for (i = 0; i < site_count && sites[i].caller != caller; i++);
        if (i == site_count && site_count < SITES) {
            sites[site_count++].caller = caller;
            void *stack[16];
            int count = backtrace(stack, 16);
            char line[80];
            int n = snprintf(line, sizeof(line), "ST alloc new pid=%ld site=%u size=%zu\n", (long)getpid(), i, size);
            write(2, line, (size_t)n);
            backtrace_symbols_fd(stack, count, 2);
        }
        unsigned h = ((uintptr_t)p >> 4) % SLOTS, slot = SLOTS;
        for (unsigned n = 0; n < SLOTS; n++, h = (h + 1) % SLOTS) {
            if (allocations[h].pointer == (void *)1 && slot == SLOTS) slot = h;
            if (!allocations[h].pointer) { if (slot == SLOTS) slot = h; break; }
        }
        if (i < SITES && slot < SLOTS) {
            allocations[slot] = (struct allocation){p, size, i};
            sites[i].live += size; sites[i].total += size; sites[i].count++;
            if (sites[i].live > sites[i].peak) sites[i].peak = sites[i].live;
        } else dropped++;
        if (++events % dump_interval == 0) dump();
    }
    pthread_mutex_unlock(&lock);
    inside = 0;
    errno = saved_errno;
}

void *malloc(size_t size) {
    void *p = __libc_malloc(size);
    record(NULL, p, size, __builtin_return_address(0));
    return p;
}
void *calloc(size_t count, size_t size) {
    void *p = __libc_calloc(count, size);
    if (!size || count <= SIZE_MAX / size)
        record(NULL, p, count * size, __builtin_return_address(0));
    return p;
}
void *realloc(void *old, size_t size) {
    void *p = __libc_realloc(old, size);
    if (p || !size) record(old, p, size, __builtin_return_address(0));
    return p;
}
void free(void *p) {
    record(p, NULL, 0, NULL);
    __libc_free(p);
}
int posix_memalign(void **result, size_t alignment, size_t size) {
    int saved_errno = errno;
    resolve_aligned();
    errno = saved_errno;
    if (!real_posix_memalign) return ENOMEM;
    int status = real_posix_memalign(result, alignment, size);
    if (!status) record(NULL, *result, size, __builtin_return_address(0));
    return status;
}
void *aligned_alloc(size_t alignment, size_t size) {
    int saved_errno = errno;
    resolve_aligned();
    errno = saved_errno;
    if (!real_aligned_alloc) { errno = ENOMEM; return NULL; }
    void *p = real_aligned_alloc(alignment, size);
    record(NULL, p, size, __builtin_return_address(0));
    return p;
}
void *memalign(size_t alignment, size_t size) {
    int saved_errno = errno;
    resolve_aligned();
    errno = saved_errno;
    if (!real_memalign) { errno = ENOMEM; return NULL; }
    void *p = real_memalign(alignment, size);
    record(NULL, p, size, __builtin_return_address(0));
    return p;
}
__attribute__((destructor)) static void finish(void) {
    inside = 1;
    pthread_mutex_lock(&lock);
    dump();
    pthread_mutex_unlock(&lock);
}
