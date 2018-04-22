#ifndef _KERNEL_MEMORY_H_
#define _KERNEL_MEMORY_H_

#include "../lib/stdint.h"
#include "../lib/kernel/bitmap.h"

struct virtual_addr
{
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

extern struct pool kernel_pool, user_pool;

void mem_init();

#endif // !_KERNEL_MEMORY_H_