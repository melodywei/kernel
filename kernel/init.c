#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"

void init_all()
{
    put_str("init_all\n");
    idt_init();
}