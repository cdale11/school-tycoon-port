/* SPDX-License-Identifier: MIT
 * Native ARM64 Linux 4.9 VM diagnostic. Trace large memory-mapping syscalls
 * and native link/frame registers, including direct Box64 syscalls.
 * Does not trace children or modify target registers/memory.
 */
#define _GNU_SOURCE
#include <elf.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

struct arm64_regs { uint64_t x[31], sp, pc, pstate; };
static volatile sig_atomic_t stopping;
static void stop(int signal) { (void)signal; stopping = 1; }

static void frames(pid_t pid, const struct arm64_regs *r) {
    fprintf(stdout," pc=%llx lr=%llx",(unsigned long long)r->pc,(unsigned long long)r->x[30]);
    uint64_t fp = r->x[29];
    for (unsigned i=0;i<12 && fp>=r->sp && fp-r->sp<8388608 && !(fp&15);i++) {
        errno=0;
        unsigned long next=(unsigned long)ptrace(PTRACE_PEEKDATA,pid,(void *)(uintptr_t)fp,0);
        if (errno) break;
        unsigned long lr=(unsigned long)ptrace(PTRACE_PEEKDATA,pid,(void *)(uintptr_t)(fp+8),0);
        if (errno) break;
        fprintf(stdout," fp%u=%lx",i,lr);
        if (next<=fp) break;
        fp=next;
    }
}

int main(int argc,char **argv) {
    char model[64]={0};
    FILE *f=fopen("/sys/firmware/devicetree/base/model","r");
    if (!f) return 88;
    fread(model,1,sizeof(model)-1,f); fclose(f);
    if (strcmp(model,"linux,dummy-virt") || argc!=2) return 88;
    char *end;
    long parsed=strtol(argv[1],&end,10);
    if (*end || parsed<=1 || parsed>2147483647) return 2;
    pid_t pid=(pid_t)parsed;
    struct sigaction action={0}; action.sa_handler=stop;
    sigaction(SIGINT,&action,NULL); sigaction(SIGTERM,&action,NULL);
    if (ptrace(PTRACE_SEIZE,pid,0,PTRACE_O_TRACESYSGOOD)<0) { perror("seize"); return 3; }
    if (ptrace(PTRACE_INTERRUPT,pid,0,0)<0) { perror("interrupt"); return 4; }
    int status;
    if (waitpid(pid,&status,0)<0) return 5;
    int deliver=0, pending=0;
    unsigned long sequence=0;
    for (;;) {
        if (stopping) { ptrace(PTRACE_DETACH,pid,0,0); break; }
        if (ptrace(PTRACE_SYSCALL,pid,0,deliver)<0) { perror("resume"); return 6; }
        deliver=0;
        pid_t waited;
        do { waited=waitpid(pid,&status,0); } while (waited<0 && errno==EINTR && !stopping);
        if (waited<0) {
            if (errno==EINTR) {
                ptrace(PTRACE_INTERRUPT,pid,0,0);
                while (waitpid(pid,&status,0)<0 && errno==EINTR);
                ptrace(PTRACE_DETACH,pid,0,0);
                break;
            }
            perror("wait"); return 7;
        }
        if (WIFEXITED(status)||WIFSIGNALED(status)) break;
        if (!WIFSTOPPED(status)) continue;
        if (WSTOPSIG(status)!=(SIGTRAP|0x80)) {
            if (WSTOPSIG(status)!=SIGTRAP) deliver=WSTOPSIG(status);
            continue;
        }
        struct arm64_regs r;
        struct iovec io={&r,sizeof(r)};
        if (ptrace(PTRACE_GETREGSET,pid,(void *)NT_PRSTATUS,&io)<0) { perror("registers"); return 8; }
        /* Linux 4.9 arm64 ptrace marks entry/exit in scratch register x7. */
        if (r.x[7]==0) {
            pending=0;
            unsigned nr=(unsigned)r.x[8];
            if ((nr==222 || nr==215 || nr==216) && r.x[1]>=131072) {
                pending=1; sequence++;
                fprintf(stdout,"ST map pid=%ld seq=%lu syscall=%u addr=%llx size=%llu arg2=%llx arg3=%llx",
                    (long)pid,sequence,nr,(unsigned long long)r.x[0],(unsigned long long)r.x[1],
                    (unsigned long long)r.x[2],(unsigned long long)r.x[3]);
                frames(pid,&r); fputc('\n',stdout); fflush(stdout);
            }
        } else if (pending) {
            fprintf(stdout,"ST map result seq=%lu value=%llx\n",sequence,(unsigned long long)r.x[0]);
            fflush(stdout); pending=0;
        }
    }
    fprintf(stdout,"ST map trace ended pid=%ld entries=%lu\n",(long)pid,sequence);
    return 0;
}
