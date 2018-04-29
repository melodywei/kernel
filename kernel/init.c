#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../device/timer.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../device/console.h"

void init_all()
{
    put_str("init_all\n");
    idt_init();
    timer_init();
    mem_init();
    thread_init();
    console_init();
}