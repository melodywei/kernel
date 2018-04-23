#include "memory.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/bitmap.h"
#include "global.h"
#include "../lib/string.h"
#include "debug.h"



// 一个页框(4KB)的位图，所能表示的内存大小为  4096 * 32 / 1024 = 128MB 
// 0xc0009f00为内核主线程的栈顶，0xc0009e00位内核主线程的pcb
// 位图地址安排在0xc0009a00，也就是说0xc0009e00 - 0xc0009a00 = 0x4000，可以存放4个页框的位图，共512MB

#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0xc0009a00
#define K_HEAP_START 0xc0100000 // 内核所用堆的起始地址，因为loader通过页表映射在了低端1MB的空间中，也就是0xc0000000~0xc000ffff映射到0x0~0xffff,所以这里需要跨过这部分空间

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

struct pool
{
    struct bitmap pool_bitmap;  // 内存池的位图结构
    uint32_t phy_addr_start; 
    uint32_t pool_size;         
};

struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   // 此结构用来给内核分配虚拟地址

// 在虚拟内存池中申请pg_cnt个虚拟页
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0;
    int bit_idx_start = -1;
    uint32_t cnt = 0;

    if(pf == PF_KERNEL)
    {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1)
        {
            return NULL;
        }

        while (cnt < pg_cnt)
        {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }

        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else
    {
        // 用户内存池
    }

    return (void *)vaddr_start;
}

// 得到虚拟地址对应的pte指针
uint32_t *pte_ptr(uint32_t vaddr)
{
    uint32_t *pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);

    return pte;
}

// 得到虚拟地址对应的pde指针
uint32_t *pde_ptr(uint32_t vaddr)
{
    uint32_t *pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);

    return pde;
}

// 在m_pool指向的物理内存池中分配一个物理页
static void *palloc(struct pool *m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1)
    {
        return NULL;
    }

    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = bit_idx * PG_SIZE + m_pool->phy_addr_start;

    return (void*)page_phyaddr;
}

// 在页表中添加虚拟地址到物理地址的映射关系
static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_phyaddr = (uint32_t)_page_phyaddr;

    uint32_t *pde = pde_ptr((uint32_t)vaddr);
    uint32_t *pte = pte_ptr((uint32_t)vaddr);

    // 在页目录内判断目录项的P位，若为1,表示该表已存在
    if(*pde & 0x01)
    {
        // 创建页表的时候，pte不应该存在
        ASSERT(!(*pte & 0x01));

        if(!(*pte & 0x01))
        {
            *pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
        }
    }
    else
    {// 页目录项不存在，此时先创建页目录项
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);

        *pde = pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
        memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);

        ASSERT(!(*pte & 0x01));
        *pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
    }
}

// 分配pg_cnt 个页空间
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);

    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if(vaddr_start == NULL)
    {
        return NULL;
    }

    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t cnt = pg_cnt;

    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    while (cnt-- > 0)
    {
        void *page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL)
        {// 此处分配失败需要释放已申请的虚拟页和物理页
            return NULL;
        }
        page_table_add((void*)vaddr, page_phyaddr);
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}

// 在内核物理内存池中申请pg_cnt页内存
void *get_kernel_pages(uint32_t pg_cnt)
{
    void *vaddr = malloc_page(PF_KERNEL, pg_cnt);

    if(vaddr != NULL)
    {
        memset(vaddr,0, pg_cnt * PG_SIZE);
    }
    return vaddr;
}


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



