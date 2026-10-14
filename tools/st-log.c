/* SPDX-License-Identifier: MIT
 * Drain a session's output into bounded rotating files without large buffers.
 * Logging failures must not terminate the game through a broken pipe.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
static int write_all(int fd,const char *p,size_t n) {
    while(n) { ssize_t w=write(fd,p,n); if(w<0 && errno==EINTR)continue;
        if(w<=0)return -1;
        p+=w;n-=(size_t)w;
    }
    return 0;
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    size_t len=strlen(argv[1]); char *previous=malloc(len+10);
    if(!previous)return 1;
    snprintf(previous,len+10,"%s.previous",argv[1]);
    int fd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0600), failed=fd<0;
    size_t bytes=0; char buffer[4096];
    for(;;) {
        ssize_t n=read(0,buffer,sizeof(buffer));
        if(n<0 && errno==EINTR)continue;
        if(n<=0) {if(n<0)failed=1;break;}
        if(fd>=0 && bytes+(size_t)n>262144) {
            close(fd);fd=-1;
            if(rename(argv[1],previous)==0) fd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0600);
            bytes=0;
            if(fd<0)failed=1;
        }
        if(fd>=0) {
            if(write_all(fd,buffer,(size_t)n)<0) {close(fd);fd=-1;failed=1;}
            else bytes+=(size_t)n;
        }
    }
    if(fd>=0)close(fd);
    if(failed)fputs("Session log write/read failed; output was drained to preserve the game.\n",stderr);
    free(previous);return failed?1:0;
}
