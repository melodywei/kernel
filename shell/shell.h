#ifndef _KERNEL_SHELL_H_
#define _KERNEL_SHELL_H_

#include "../fs/fs.h"

void print_prompt(void);
void my_shell(void);
extern char final_path[MAX_PATH_LEN];
#endif
