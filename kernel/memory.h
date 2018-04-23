#ifndef _KERNEL_MEMORY_H_
#define _KERNEL_MEMORY_H_

#include "../lib/stdint.h"
#include "../lib/kernel/bitmap.h"

enum pool_flags
{
    PF_KERNEL=1,
    PF_USER
};

// 页表或页目录存在位
#define PG_P_1 1
#define PG_P_0 0

// R/W属性位
#define PG_RW_R 0
#define PG_RW_W 2

// 用户级/系统级 属性位
#define PG_US_S 0
#define PG_US_U 4

struct virtual_addr
{
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

extern struct pool kernel_pool, user_pool;

void mem_init();

#endif // !_KERNEL_MEMORY_H_