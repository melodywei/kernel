#ifndef _DEVICE_CONSOLE_H_
#define _DEVICE_CONSOLE_H_
#include "../lib/stdint.h"

void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_str(char* str);
void console_put_char(uint8_t char_asci);
void console_put_int(uint32_t num);

#endif //!_DEVICE_CONSOLE_H_
