#ifndef _KERNEL_MEMORY_H_
#define _KERNEL_MEMORY_H_

#include "../lib/stdint.h"
#include "../lib/kernel/bitmap.h"
#include "../lib/kernel/list.h"


// 页表或页目录存在位
#define PG_P_1 1
#define PG_P_0 0

// R/W属性位
#define PG_RW_R 0
#define PG_RW_W 2

// 用户级/系统级 属性位
#define PG_US_S 0
#define PG_US_U 4

// 内存块描述符个数
#define DESC_CNT 7


enum pool_flags
{
    PF_KERNEL=1,
    PF_USER
};

struct mem_block
{
    struct list_elem free_elem;
};

// 内存块描述符
struct mem_block_desc
{
    uint32_t block_size;    // 内存块规格
    uint32_t blocks_per_arena;  //本arena中可容纳mem_block的数量
    struct list free_list;  // 目前可用的mem_block链表
};

struct virtual_addr
{
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

extern struct pool kernel_pool, user_pool;


void mem_init(void);
void* get_kernel_pages(uint32_t pg_cnt);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void malloc_init(void);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
void* get_user_pages(uint32_t pg_cnt);
void block_desc_init(struct mem_block_desc *desc_array);
void *sys_malloc(uint32_t size);
void pfree(uint32_t pg_phy_addr);
void sys_free(void* ptr);
void *get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr);
void free_a_phy_addr(uint32_t pg_phy_addr);
#endif // !_KERNEL_MEMORY_H_