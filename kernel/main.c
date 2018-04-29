#include "../lib/kernel/print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"
#include "interrupt.h"
#include "../device/console.h"

void k_thread(void *);

int main() 
{
    put_str("I am kernel\n");
    init_all();
    
    thread_start("k_thread_a", 31, k_thread, "argA ");
    
    thread_start("k_thread_b", 8, k_thread, "argB ");

    intr_enable();

    while(1)
    {
        console_put_str("Main ");
    }

    return 0;
}

void k_thread(void *arg)
{
    char *para = (char *)arg;
    while(true)
    {
        console_put_str(arg);
    }
}
