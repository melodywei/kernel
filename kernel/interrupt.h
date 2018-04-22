#ifndef _KERNEL_INTERRUPT_H_
#define _KERNEL_INTERRUPT_H_

#include "../lib/stdint.h"

typedef void * intr_handler;


void idt_init();

#endif //!_KERNEL_INTERRUPT_H_