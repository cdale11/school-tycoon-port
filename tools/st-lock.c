/* SPDX-License-Identifier: MIT
 * Serialize a launcher with a process-owned POSIX lock surviving exec.
 * Unlike flock, forked Wine/background children do not inherit lock ownership.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
int main(int argc,char **argv) {
    if(argc<3){fputs("Usage: st-lock FILE COMMAND [ARGS...]\n",stderr);return 2;}
    int fd=open(argv[1],O_RDWR|O_CREAT|O_NOFOLLOW|O_NONBLOCK,0600);
    struct stat st;
    if(fd<0 || fstat(fd,&st)<0 || !S_ISREG(st.st_mode)) {
        fputs("Cannot open a regular session lock file.\n",stderr);return 1;
    }
    struct flock lock={.l_type=F_WRLCK,.l_whence=SEEK_SET};
    if(fcntl(fd,F_SETLK,&lock)<0) {
        if(errno==EACCES || errno==EAGAIN)fputs("School Tycoon already has a running launcher.\n",stderr);
        else perror("Session lock");
        return 1;
    }
    char owner[32];snprintf(owner,sizeof(owner),"%ld",(long)getpid());
    if(setenv("ST_SESSION_LOCKED",owner,1)<0){perror("Session environment");return 1;}
    execvp(argv[2],argv+2);
    perror("Session command");return 1;
}
