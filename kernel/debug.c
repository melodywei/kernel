#include "debug.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"


void panic_spin(char *filename, int line, const char *func, const char *condition)
{
    intr_disable();

    put_str("\n\n\n !!!! error !!!!\n");
    put_str("filename: ");put_str(filename);put_str("\n");
    put_str("line: ");put_int(line);put_str("\n");
    put_str("func: ");put_str(func);put_str("\n");
    put_str("condition: ");put_str(condition);put_str("\n");
    while(1);
}

