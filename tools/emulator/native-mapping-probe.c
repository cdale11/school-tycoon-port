/* SPDX-License-Identifier: MIT
 * Diagnostic preload: observe native mmap after a PE32 graphics call under
 * Box64/Wine. Place before the production graphics preloads; not shipped.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <stdatomic.h>
int crusty_glXMakeCurrent(void *d,unsigned long w,void *c) {
 int (*next)(void*,unsigned long,void*)=dlsym(RTLD_NEXT,"crusty_glXMakeCurrent");
 static atomic_flag checked=ATOMIC_FLAG_INIT;
 if(c && !atomic_flag_test_and_set(&checked)) {
  void *p=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  fprintf(stderr,"ST-MAPPING-POLICY mmap32=%s address=%p errno=%d\n",getenv("BOX64_MMAP32")?getenv("BOX64_MMAP32"):"default",p,p==MAP_FAILED?errno:0);
  if(p!=MAP_FAILED)munmap(p,4096);
 }
 return next && next!=crusty_glXMakeCurrent?next(d,w,c):0;
}
