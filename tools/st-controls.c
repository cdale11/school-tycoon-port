/* School Tycoon controller -> private X11 session. SPDX-License-Identifier: MIT
 * SDL2 controller API and XTest are loaded from the installed native runtime.
 * No uinput devices or global process-name kill commands are used. */
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "st-menu.h"

static volatile sig_atomic_t running = 1;
static void stop(int sig) { (void)sig; running = 0; }
#define FN(lib, ret, name, args) ret (*name) args = (ret (*) args)dlsym(lib, #name); if (!name) { fprintf(stderr,"Missing %s\n",#name); return 1; }
int main(void) {
    void *s = dlopen("libSDL2-2.0.so.0", RTLD_NOW|RTLD_LOCAL);
    void *x = dlopen("libX11.so.6", RTLD_NOW|RTLD_LOCAL);
    void *t = dlopen("libXtst.so.6", RTLD_NOW|RTLD_LOCAL);
    if (!s || !x || !t) { fprintf(stderr,"Native controller libraries: %s\n",dlerror()); return 1; }
    FN(s, int, SDL_Init, (unsigned));
    FN(s, void, SDL_Quit, (void));
    FN(s, int, SDL_NumJoysticks, (void));
    FN(s, int, SDL_IsGameController, (int));
    FN(s, void *, SDL_GameControllerOpen, (int));
    FN(s, void, SDL_GameControllerClose, (void *));
    FN(s, int, SDL_GameControllerGetAttached, (void *));
    FN(s, unsigned char, SDL_GameControllerGetButton, (void *, int));
    FN(s, short, SDL_GameControllerGetAxis, (void *, int));
    FN(s, void, SDL_GameControllerUpdate, (void));
    FN(s, int, SDL_GameControllerAddMapping, (const char *));
    FN(s, int, SDL_GameControllerGetButtonFromString, (const char *));
    FN(x, void *, XOpenDisplay, (const char *));
    FN(x, int, XCloseDisplay, (void *));
    FN(x, unsigned char, XKeysymToKeycode, (void *, unsigned long));
    FN(x, int, XFlush, (void *));
    FN(x, unsigned long, XDefaultRootWindow, (void *));
    FN(x, int, XQueryPointer, (void *, unsigned long, unsigned long *, unsigned long *, int *, int *, int *, int *, unsigned *));
    FN(t, int, XTestFakeButtonEvent, (void *, unsigned, int, unsigned long));
    FN(t, int, XTestFakeKeyEvent, (void *, unsigned, int, unsigned long));
    FN(t, int, XTestFakeRelativeMotionEvent, (void *, int, int, unsigned long));
    FN(t, int, XTestFakeMotionEvent, (void *, int, int, int, unsigned long));
    if (SDL_Init(0x2000) != 0) { fputs("SDL controller init failed\n",stderr); return 1; }
    const char *db = getenv("SDL_GAMECONTROLLERCONFIG_FILE");
    FILE *f = db ? fopen(db,"r") : NULL;
    if (f) { char line[4096]; while(fgets(line,sizeof(line),f)) if(line[0]!='#') SDL_GameControllerAddMapping(line); fclose(f); }
    void *display = XOpenDisplay(NULL);
    if (!display) { SDL_Quit(); fputs("X display unavailable\n",stderr); return 1; }
    void *pad = NULL;
    int old[8] = {0};
    const int buttons[6] = {0,1,2,3,6,10}; /* A B X Y Start R1 */
    int hotkey = 5; /* SDL Guide: KNULLI/PortMaster's mapped Fn button. */
    const char *configured_hotkey = getenv("HOTKEY");
    if (configured_hotkey && *configured_hotkey) {
        const char *name = !strcmp(configured_hotkey,"l3") ? "leftstick" :
                           !strcmp(configured_hotkey,"r3") ? "rightstick" : configured_hotkey;
        int mapped = SDL_GameControllerGetButtonFromString(name);
        if (mapped >= 0 && mapped != 6) hotkey = mapped;
        else fprintf(stderr,"Unknown HOTKEY %s; using Guide and Select exit shortcuts\n", configured_hotkey);
    }
    fprintf(stderr,"Controller display=%s, mapping=%s, exit=Guide/Select/button%d + Start\n",getenv("DISPLAY")?getenv("DISPLAY"):"unset",db?db:"unset",hotkey);
    const unsigned long keys[8] = {0,0,0x20,0xff1b,0xff0d,0xff09,0xff55,0xff56};
    signal(SIGTERM,stop); signal(SIGINT,stop);
    struct timespec tick = {0,16000000};
    int requested_exit = 0;
    int menu_fd=st_menu_open(), selected=-1, old_dx=0, old_dy=0;
    const struct st_menu_layout *menu=NULL, *observed=NULL;
    int resolution_ok=0, refresh=0, pointer_started=0, image_reported=0;
    while(running) {
        SDL_GameControllerUpdate();
        if (pad && !SDL_GameControllerGetAttached(pad)) { fputs("Controller disconnected\n",stderr); for(int i=0;i<8;i++) if(old[i]) {
            if(i<2) XTestFakeButtonEvent(display,i==0?1:3,0,0);
            else XTestFakeKeyEvent(display,XKeysymToKeycode(display,keys[i]),0,0);
            old[i]=0;
        }
        XFlush(display);SDL_GameControllerClose(pad); pad = NULL;
        selected=-1; old_dx=old_dy=0; observed=menu=NULL; resolution_ok=0; }
        if (!pad) for(int i=0;i<SDL_NumJoysticks();i++) if(SDL_IsGameController(i)) { pad=SDL_GameControllerOpen(i); if(pad) {fputs("Controller connected\n",stderr);break;} }
        if(pad && SDL_GameControllerGetButton(pad,6) &&
           (SDL_GameControllerGetButton(pad,5) || SDL_GameControllerGetButton(pad,4) ||
            SDL_GameControllerGetButton(pad,hotkey))) { requested_exit=1; break; }
        int now[8] = {0};
        if(pad) {
            for(int i=0;i<6;i++) now[i]=!!SDL_GameControllerGetButton(pad,buttons[i]);
            now[6]=SDL_GameControllerGetAxis(pad,4)>16000;
            now[7]=SDL_GameControllerGetAxis(pad,5)>16000;
            /* /proc/PID/mem refers to an address space, not a PID. Wine can
             * exec after we open it; periodically reopen an unreadable image. */
            if(refresh++%60==0) {
                menu_fd=st_menu_refresh(menu_fd);
                if(!image_reported && st_menu_u32(menu_fd,0x400000)==0x00905a4d) {
                    fputs("Menu observer: game image readable\n",stderr);image_reported=1;
                }
                resolution_ok=st_menu_resolution();
            }
            const struct st_menu_layout *current=st_menu_read(menu_fd);
            if(current!=observed) {resolution_ok=st_menu_resolution();observed=current;}
            if(!resolution_ok) current=NULL;
            if (menu!=current) {menu=current;selected=-1;
                fprintf(stderr,"D-pad mode: %s\n",menu?"menu selection":"pointer");
            }
            int precision=SDL_GameControllerGetButton(pad,9);
            int step=precision?1:5;
            int dx=step*(!!SDL_GameControllerGetButton(pad,14)-!!SDL_GameControllerGetButton(pad,13));
            int dy=step*(!!SDL_GameControllerGetButton(pad,12)-!!SDL_GameControllerGetButton(pad,11));
            if(menu && !precision) {
                if((dx||dy) && (dx!=old_dx||dy!=old_dy)) {
                    selected=st_menu_next(menu,selected,(dx>0)-(dx<0),dx?0:(dy>0)-(dy<0));
                    pointer_started=1;
                    XTestFakeMotionEvent(display,-1,menu->points[selected].x,menu->points[selected].y,0);
                }
            } else if(dx||dy) {
                selected=-1;
                if(resolution_ok) {
                    unsigned long root,child;unsigned mask;int rx,ry,wx,wy;
                    if(XQueryPointer(display,XDefaultRootWindow(display),&root,&child,&rx,&ry,&wx,&wy,&mask)) {
                        if(!pointer_started || rx<0 || rx>=800 || ry<0 || ry>=600) {
                            rx=400;ry=300;pointer_started=1;
                            fputs("Pointer initialized inside 800x600 game area\n",stderr);
                        }
                        rx+=dx;ry+=dy;
                        if(rx<0) rx=0;
                        if(rx>799) rx=799;
                        if(ry<0) ry=0;
                        if(ry>599) ry=599;
                        XTestFakeMotionEvent(display,-1,rx,ry,0);
                    }
                } else XTestFakeRelativeMotionEvent(display,dx,dy,0);
            }
            old_dx=dx; old_dy=dy;
        }
        for(int i=0;i<8;i++) if(now[i]!=old[i]) {
            if(i<2) XTestFakeButtonEvent(display,i==0?1:3,now[i],0);
            else XTestFakeKeyEvent(display,XKeysymToKeycode(display,keys[i]),now[i],0);
            old[i]=now[i];
        }
        XFlush(display); nanosleep(&tick,NULL);
    }
    for(int i=0;i<8;i++) if(old[i]) {
        if(i<2) XTestFakeButtonEvent(display,i==0?1:3,0,0);
        else XTestFakeKeyEvent(display,XKeysymToKeycode(display,keys[i]),0,0);
    }
    XFlush(display); XCloseDisplay(display);
    if(pad) SDL_GameControllerClose(pad);
    if(menu_fd>=0) close(menu_fd);
    SDL_Quit(); return requested_exit?0:1;
}
