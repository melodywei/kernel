#include "../lib/kernel/print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"

void k_thread(void *);

int main() 
{
    put_str("I am kernel\n");
    init_all();
    
    thread_start("k_thread_a", 31, k_thread, "argA ");
    


    while(1);

    return 0;
}

void k_thread(void *arg)
{
    char *para = (char *)arg;
    while(true)
    {
        put_str(para);
    }
}
