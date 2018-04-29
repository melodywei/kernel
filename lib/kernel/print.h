#ifndef _LIB_KERNEL_PRINT_H_
#define _LIB_KERNEL_PRINT_H_

#include "../stdint.h"

void put_char(const char ch_asci);
void put_str(const char *str);
void put_int(unsigned int num);
void set_cursor(uint32_t cursor_pos);

#endif //!_LIB_KERNEL_PRINT_H_