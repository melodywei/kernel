#ifndef _DEVICE_TIMER_H_
#define _DEVICE_TIMER_H_

#include "../lib/stdint.h"

void timer_init();
void mtime_sleep(uint32_t m_seconds);

#endif //!_DEVICE_TIMER_H_