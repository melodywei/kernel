#ifndef _LIB_USER_SYSCALL_H_
#define _LIB_USER_SYSCALL_H_


#include "../stdint.h"

enum SYSCALL_NR 
{
   SYS_GETPID
};

uint32_t getpid(void);
#endif

