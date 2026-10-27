"""Exercise bounded read-only screen detection and spatial button selection."""
from pathlib import Path
import subprocess
import tempfile
repo = Path(__file__).resolve().parents[1]
source = r'''
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "st-menu.h"
static void word(int fd, unsigned offset, unsigned value) {
 unsigned char b[4]={value,value>>8,value>>16,value>>24};
 assert(pwrite(fd,b,4,offset)==4);
}
int main(void) {
 unsetenv("ST_MENU_SETTINGS");assert(!st_menu_resolution());
 char settings[]="/tmp/st-settings-XXXXXX";int sf=mkstemp(settings);assert(sf>=0);
 word(sf,0,30);word(sf,40,800);word(sf,44,600);
 setenv("ST_MENU_SETTINGS",settings,1);assert(st_menu_resolution());
 word(sf,40,1024);assert(!st_menu_resolution());close(sf);unlink(settings);

 unsetenv("ST_MENU_GAME_PID"); assert(st_menu_open()==-1);
 setenv("ST_MENU_GAME_PID","1/../../bad",1);assert(st_menu_open()==-1);
 char path[]="/tmp/st-menu-XXXXXX";int fd=mkstemp(path);assert(fd>=0);unlink(path);
 assert(st_menu_read(fd)==NULL);
 word(fd,0x400000,0x00905a4d);word(fd,0x6548c4,0x100000);
 word(fd,0x100000,0x5febd4);word(fd,0x1000a0,0x110000);word(fd,0x1000a4,0x110004);
 word(fd,0x110000,0x120000);word(fd,0x120000,0x6002b4);
 const struct st_menu_layout *m=st_menu_read(fd);assert(m && m->count==5);
 assert(st_menu_next(m,-1,0,1)==0);assert(st_menu_next(m,0,0,1)==1);
 assert(st_menu_next(m,1,0,-1)==0);assert(st_menu_next(m,3,1,0)==4);
 assert(st_menu_next(m,4,-1,0)==3);assert(st_menu_next(m,0,0,-1)==0);
 word(fd,0x120000,0x600264);m=st_menu_read(fd);assert(m && m->points[1].y==292);
 word(fd,0x120000,0x123456);assert(!st_menu_read(fd));
 word(fd,0x120000,0x6002b4);word(fd,0x1000a4,0x110008);assert(!st_menu_read(fd));
 word(fd,0x6548c4,0xfffffffe);assert(!st_menu_read(fd));
 close(fd);assert(!st_menu_read(fd));
 puts("PASS: menu geometry, direction, unknown/modal screens, invalid PID and pointers");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp)/'menu.c'; c.write_text(source)
    exe = Path(tmp)/'menu'
    subprocess.run(['cc','-Wall','-Wextra','-Werror','-I',str(repo/'tools'),str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
