/* SPDX-License-Identifier: MIT
 * Delayed native mapping fixture for the ARM64 VM's syscall tracer.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
int main(void) {
    sleep(5);
    volatile unsigned char *p=mmap(NULL,1048576,PROT_READ|PROT_WRITE,
                                  MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (p==MAP_FAILED) return 1;
    p[0]=42; p[1048575]=43;
    if (munmap((void *)p,1048576)) return 2;
    puts("ST-MAP-FIXTURE-PASS");
    return 0;
}
