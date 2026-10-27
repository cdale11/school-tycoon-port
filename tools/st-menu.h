/* SPDX-License-Identifier: MIT. Read-only menu observer for the verified 2004
 * executable. Unknown screens, overlays and unavailable /proc access retain
 * pointer controls. Never writes game memory or changes a game file. */
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

struct st_menu_point { int x, y; };
struct st_menu_layout { uint32_t vtable; int count; struct st_menu_point points[5]; };
static const struct st_menu_layout st_menus[] = {
    {0x6002b4, 5, {{400,187},{400,286},{400,390},{175,545},{670,545}}},
    {0x600264, 5, {{400,210},{400,292},{400,375},{400,465},{123,550}}},
    {0x6003a4, 1, {{123,550}}},
};
static uint32_t st_menu_u32(int fd, uint32_t address) {
    unsigned char b[4];
    if (address < 0x10000 || address > 0x7ffffffb ||
        pread(fd,b,4,(off_t)address)!=4) return 0;
    return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
}
static const struct st_menu_layout *st_menu_read(int fd) {
    if (fd<0 || st_menu_u32(fd,0x400000)!=0x00905a4d) return NULL;
    uint32_t manager=st_menu_u32(fd,0x6548c4);
    if (manager>0x7fffff00 || st_menu_u32(fd,manager)!=0x5febd4) return NULL;
    uint32_t begin=st_menu_u32(fd,manager+0xa0), end=st_menu_u32(fd,manager+0xa4);
    /* Never navigate through an unknown modal overlay. */
    if (!begin || begin>0x7ffffffb || end!=begin+4) return NULL;
    uint32_t vtable=st_menu_u32(fd,st_menu_u32(fd,begin));
    for (unsigned i=0;i<sizeof(st_menus)/sizeof(st_menus[0]);i++)
        if (st_menus[i].vtable==vtable) return &st_menus[i];
    return NULL;
}
static int st_menu_open(void) {
    const char *pid=getenv("ST_MENU_GAME_PID");
    if (!pid || !*pid) return -1;
    char *end; long value=strtol(pid,&end,10);
    if (*end || value<=1 || value>INT_MAX) return -1;
    char path[64]; snprintf(path,sizeof(path),"/proc/%ld/mem",value);
    int fd=open(path,O_RDONLY|O_CLOEXEC);
    static int reported=-2;
    int status=fd<0?-1:0;
    if(status!=reported) fprintf(stderr,"Menu observer: %s; unknown screens use pointer controls\n",fd<0?"unavailable":"read-only");
    reported=status;
    return fd;
}
static int st_menu_refresh(int fd) {
    if(st_menu_u32(fd,0x400000)==0x00905a4d) return fd;
    if(fd>=0) close(fd);
    return st_menu_open();
}
static int st_menu_resolution(void) {
    const char *path=getenv("ST_MENU_SETTINGS");
    if (!path || !*path) return 0;
    FILE *file=fopen(path,"rb");
    if (!file) return 0;
    unsigned char b[48];size_t size=fread(b,1,sizeof(b),file);fclose(file);
    return size==sizeof(b) && b[0]==30 && !b[1] && !b[2] && !b[3] &&
        b[40]==0x20 && b[41]==3 && !b[42] && !b[43] &&
        b[44]==0x58 && b[45]==2 && !b[46] && !b[47];
}
static int st_menu_next(const struct st_menu_layout *menu, int selected, int dx, int dy) {
    if (selected<0 || selected>=menu->count) return 0;
    int best=selected; long score=LONG_MAX;
    for(int i=0;i<menu->count;i++) {
        int x=menu->points[i].x-menu->points[selected].x;
        int y=menu->points[i].y-menu->points[selected].y;
        int forward=dx?x*dx:y*dy, side=dx?y:x;
        if (forward<=0) continue;
        long candidate=(long)forward*forward+16L*side*side;
        if(candidate<score) {score=candidate;best=i;}
    }
    return best;
}
