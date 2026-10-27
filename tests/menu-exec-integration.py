"""A proc mem fd opened before Wine exec must be replaced, not kept forever."""
import subprocess, tempfile
from pathlib import Path
repo=Path(__file__).resolve().parents[1]
source=r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include "st-menu.h"
static void image(void){
 void*p=mmap((void*)0x400000,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
 assert(p==(void*)0x400000);*(uint32_t*)p=0x00905a4d;
}
int main(int argc,char**argv){
 if(argc==2){image();int fd=atoi(argv[1]);assert(write(fd,"R",1)==1);for(;;)pause();}
 int ready[2],go[2];assert(pipe(ready)==0&&pipe(go)==0);
 pid_t child=fork();assert(child>=0);
 if(!child){image();assert(write(ready[1],"R",1)==1);char c;assert(read(go[0],&c,1)==1);
 char fd[32];snprintf(fd,sizeof(fd),"%d",ready[1]);execl("/proc/self/exe","fixture",fd,NULL);_exit(1);}
 char c;assert(read(ready[0],&c,1)==1);
 char pid[32];snprintf(pid,sizeof(pid),"%d",child);setenv("ST_MENU_GAME_PID",pid,1);
 int fd=st_menu_open();assert(fd>=0&&st_menu_u32(fd,0x400000)==0x00905a4d);
 assert(st_menu_refresh(fd)==fd);
 assert(write(go[1],"G",1)==1);assert(read(ready[0],&c,1)==1);
 assert(st_menu_u32(fd,0x400000)==0); /* old address space has gone */
 fd=st_menu_refresh(fd);assert(fd>=0&&st_menu_u32(fd,0x400000)==0x00905a4d);
 close(fd);kill(child,SIGTERM);waitpid(child,NULL,0);
 (void)st_menu_read(-1);(void)st_menu_resolution();(void)st_menu_next(&st_menus[0],-1,0,1);
 puts("PASS: real exec invalidates proc mem; observer reopens the current game image");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(source)
 subprocess.run(['cc','-fPIE','-pie','-Wall','-Wextra','-Werror','-I',str(repo/'tools'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True,timeout=10)
