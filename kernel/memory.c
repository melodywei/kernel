#include "memory.h"
#include "../lib/kernel/print.h"



// 一个页框(4KB)的位图，所能表示的内存大小为  4096 * 32 / 1024 = 128MB 
// 0xc0009f00为内核主线程的栈顶，0xc0009e00位内核主线程的pcb
// 位图地址安排在0xc0009a00，也就是说0xc0009e00 - 0xc0009a00 = 0x4000，可以存放4个页框的位图，共512MB

#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0xc0009a00
#define K_HEAP_START 0xc0100000 // 内核所用堆的起始地址，因为loader通过页表映射在了低端1MB的空间中，也就是0xc0000000~0xc000ffff映射到0x0~0xffff,所以这里需要跨过这部分空间

struct pool
{
    struct bitmap pool_bitmap;  // 内存池的位图结构
    uint32_t phy_addr_start; 
    uint32_t pool_size;         
};

struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   // 此结构用来给内核分配虚拟地址

// 初始化内存池
static void mem_pool_init(uint32_t all_mem)
{
    put_str("   mem_pool_init start\n");

    uint32_t page_table_size = PG_SIZE * 256;

    uint32_t used_mem = page_table_size + 0x100000;

    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;

    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    // 内核的位图大小，在位图中，1bit表示1页
    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;

    // 内核内存池的起始地址
    uint32_t kp_start = used_mem;

    // 用户内存池的起始地址
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

    // 输出内存信息
    put_str("      kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("      user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    // 将位图置0
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}

void mem_init()
{
    put_str("mem_init start\n");

    // 物理内存的大小放在地址0xb00处
    uint32_t mem_bytes_total = *((uint32_t*)0xb00);

    mem_pool_init(mem_bytes_total);

    put_str("mem_init done\n");
}



