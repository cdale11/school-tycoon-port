/* SPDX-License-Identifier: MIT
 * VM-only: keep native SDL presentation separate from Wine input windows.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *XOpenDisplay(const char *name) {
 const char *backend=getenv("ST_SDL_DISPLAY"); Dl_info caller={0};
 if(backend && dladdr(__builtin_return_address(0),&caller) && caller.dli_fname && strstr(caller.dli_fname,"libSDL2")) {
  fprintf(stderr,"ST VM: SDL display %s (Wine remains on %s)\n",backend,getenv("DISPLAY"));name=backend;
 }
 void *lib=dlopen("libX11.so.6",RTLD_NOW|RTLD_NOLOAD);
 void *(*next)(const char*)=lib?dlsym(lib,"XOpenDisplay"):0;
 if(!next || next==XOpenDisplay)return 0;
 void *display=next(name); if(lib)dlclose(lib);return display;
}
