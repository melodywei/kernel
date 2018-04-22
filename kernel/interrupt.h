#ifndef _KERNEL_INTERRUPT_H_
#define _KERNEL_INTERRUPT_H_

#include "../lib/stdint.h"

typedef void * intr_handler;

enum intr_status
{
    INTR_OFF,
    INTR_ON
};

void idt_init();

// 通过EFLAGS中的if位获取当前中断状态
enum intr_status intr_get_status();

// 开中断，返回中断前的状态
enum intr_status intr_enable();

// 关中断，返回中断前的状态
enum intr_status intr_disable();

// 设置中断状态
enum intr_status intr_set_stauts(enum intr_status status);

#endif //!_KERNEL_INTERRUPT_H_