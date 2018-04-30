#ifndef _USERPROG_PROCESS_H_
#define _USERPROG_PROCESS_H_

#include "../thread/thread.h"
#include "../lib/stdint.h"


#define default_prio 31
#define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000


void process_execute(void* filename, char* name);
void start_process(void* filename_);
void process_activate(task_struct* p_thread);
void page_dir_activate(task_struct* p_thread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(task_struct* user_prog);


#endif // !_USERPROG_PROCESS_H_
