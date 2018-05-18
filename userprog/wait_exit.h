#ifndef _USERPROG_WAITEXIT_H_
#define _USERPROG_WAITEXIT_H_
#include "../thread/thread.h"

pid_t sys_wait(int32_t* status);
void sys_exit(int32_t status);

#endif 
