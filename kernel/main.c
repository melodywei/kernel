#include "../lib/kernel/print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"

int main() 
{
    put_str("I am kernel\n");
    init_all();
    void *vaddr = get_kernel_pages(3);
    put_str("\n get_kernel_page start vaddr is: ");
    put_int((uint32_t)vaddr);
    


    while(1);

    return 0;
}

