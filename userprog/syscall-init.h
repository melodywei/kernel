#ifndef _USERPROG_SYSCALLINIT_H_
#define _USERPROG_SYSCALLINIT_H_

#include "../lib/stdint.h"

void syscall_init(void);
uint32_t sys_getpid(void);

#endif
