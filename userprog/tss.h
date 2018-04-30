#ifndef _USERPROG_TSS_H_
#define _USERPROG_TSS_H_

#include "../thread/thread.h"

void update_tss_esp(task_struct* pthread);
void tss_init(void);

#endif //!_USERPROG_TSS_H_
