#include "thread.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../lib/kernel/list.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"
#include "../kernel/memory.h"
#include "../lib/kernel/print.h"
#include "../userprog/process.h"
#include "../thread/sync.h"
#include "../lib/kernel/bitmap.h"

task_struct *main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem *thread_tag;

task_struct *idle_thread;

struct lock pid_lock;
uint8_t pid_bitmap_bits[128] = {0};

struct pid_pool
{
    struct bitmap pid_bitmap;
    uint32_t pid_start;  // 起始pid
    struct lock pid_lock;
}pid_pool;

extern void switch_to(task_struct *cur, task_struct *next);
extern void init();

static void idle(void *arg UNUSED)
{
    while (1)
    {
        thread_block(TASK_BLOCKED);
        asm volatile("sti; hlt" ::: "memory");
    }
}

static void pid_pool_init()
{
    pid_pool.pid_start = 1;
    pid_pool.pid_bitmap.bits = pid_bitmap_bits;
    pid_pool.pid_bitmap.btmp_bytes_len = 128;
    bitmap_init(&pid_pool.pid_bitmap);
    lock_init(&pid_pool.pid_lock);
}

/* 分配pid */
static pid_t allocate_pid(void)
{
    lock_acquire(&pid_pool.pid_lock);
    int32_t bit_idx = bitmap_scan(&pid_pool.pid_bitmap, 1);
    bitmap_set(&pid_pool.pid_bitmap, bit_idx, 1);
    lock_release(&pid_pool.pid_lock);
    return (bit_idx + pid_pool.pid_start);
}

/* 释放pid */
void release_pid(pid_t pid)
{
    lock_acquire(&pid_pool.pid_lock);
    int32_t bit_idx = pid - pid_pool.pid_start;
    bitmap_set(&pid_pool.pid_bitmap, bit_idx, 0);
    lock_release(&pid_pool.pid_lock);
}

/* fork进程时为其分配pid*/
pid_t fork_pid(void)
{
    return allocate_pid();
}

task_struct *running_thread()
{
    uint32_t esp;

    asm ("mov %%esp, %0" : "=g" (esp));
    return (task_struct *)(esp & 0xfffff000);
}

static void kernel_thread(thread_func *function, void *func_arg)
{
    intr_enable();
    function(func_arg);
}

void thread_create(task_struct* pthread, thread_func function, void* func_arg)
{
     // 先预留中断使用栈的空间
    pthread->self_kstack -= sizeof(intr_stack);

     // 再留出线程栈空间,可见thread.h中定义
    pthread->self_kstack -= sizeof(thread_stack);

    thread_stack* kthread_stack = (thread_stack*)pthread->self_kstack; 
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0; 
}


// 初始化线程基本信息
void init_thread(task_struct* pthread, char* name, int prio) 
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->pid = allocate_pid();

    if (pthread == main_thread)
    {
        pthread->status = TASK_RUNNING;
    }
    else
    {
        pthread->status = TASK_READY;
    }

    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;

    // self_kstack是线程自己在内核态下使用的栈顶地址 
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);

    pthread->fd_table[0] = 0;
    pthread->fd_table[1] = 1;
    pthread->fd_table[2] = 2;

    uint8_t fd_idx = 3;

    while(fd_idx < MAX_FILES_OPEN_PER_PROC)
    {
        pthread->fd_table[fd_idx++] = -1;
    }


    pthread->stack_magic = 0x19971234;     // 自定义的魔数，检查栈溢出
}

// 创建一优先级为prio的线程,线程名为name,线程所执行的函数是function(func_arg) 
task_struct* thread_start(char* name, int prio, thread_func function, void*  func_arg) 
{ 
    // pcb都位于内核空间,包括用户进程的pcb也是在内核空间
    task_struct* thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    /* 确保之前不在队列中 */
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    /* 加入就绪线程队列 */
    list_append(&thread_ready_list, &thread->general_tag);

    /* 确保之前不在队列中 */
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    /* 加入全部线程队列 */
    list_append(&thread_all_list, &thread->all_list_tag);

    //asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret"  : : "g" (thread->self_kstack) : "memory");     
    return thread;  
}

static void make_main_thread()
{
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule()
{
    ASSERT(intr_get_status() == INTR_OFF);

    task_struct* cur = running_thread();

    if (cur->status == TASK_RUNNING) 
    { 
        // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;     // 重新将当前线程的ticks再重置为其priority;
        cur->status = TASK_READY;
    } 
    else 
    {
       /* 若此线程需要某事件发生后才能继续上cpu运行,
       不需要将其加入队列,因为当前线程不在就绪队列中。*/
    }

    if(list_empty(&thread_ready_list))
    {
        thread_unblock(idle_thread);
    }
  
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;     // thread_tag清空
    /* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
    thread_tag = list_pop(&thread_ready_list);
    task_struct* next = elem2entry(task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;

    process_activate(next);
    switch_to(cur, next);

}

/* 当前线程将自己阻塞,标志其状态为stat. */
void thread_block(task_status stat) 
{
    /* stat取值为TASK_BLOCKED,TASK_WAITING,TASK_HANGING,也就是只有这三种状态才不会被调度*/
    ASSERT(stat == TASK_BLOCKED || stat == TASK_WAITING || stat == TASK_HANGING);

    enum intr_status old_status = intr_disable();
    task_struct* cur_thread = running_thread();

    cur_thread->status = stat; // 置其状态为stat 
    schedule();            // 将当前线程换下处理器
    
    /* 待当前线程被解除阻塞后才继续运行下面的intr_set_status */
   intr_set_status(old_status);
}

/* 将线程pthread解除阻塞 */
void thread_unblock(task_struct* pthread) 
{
    enum intr_status old_status = intr_disable();
    ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING));

    if (pthread->status != TASK_READY) 
    {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));

        if (elem_find(&thread_ready_list, &pthread->general_tag)) 
        {
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }   
        
        list_push(&thread_ready_list, &pthread->general_tag);    // 放到队列的最前面,使其尽快得到调度
        pthread->status = TASK_READY;
   }   
   intr_set_status(old_status);
}

void thread_yield()
{
    task_struct *cur = running_thread();
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));

    list_append(&thread_ready_list, &cur->general_tag);
    cur->status = TASK_READY;
    schedule();
    intr_set_status(old_status);
}

/* 回收thread_over的pcb和页表,并将其从调度队列中去除 */
void thread_exit(task_struct *thread_over, bool need_schedule)
{
    /* 要保证schedule在关中断情况下调用 */
    intr_disable();
    thread_over->status = TASK_DIED;

    /* 如果thread_over不是当前线程,就有可能还在就绪队列中,将其从中删除 */
    if (elem_find(&thread_ready_list, &thread_over->general_tag))
    {
        list_remove(&thread_over->general_tag);
    }
    if (thread_over->pgdir)
    { 
        // 如是进程,回收进程的页表
        mfree_page(PF_KERNEL, thread_over->pgdir, 1);
    }

    /* 从all_thread_list中去掉此任务 */
    list_remove(&thread_over->all_list_tag);

    /* 回收pcb所在的页,主线程的pcb不在堆中,跨过 */
    if (thread_over != main_thread)
    {
        mfree_page(PF_KERNEL, thread_over, 1);
    }

    /* 归还pid */
    release_pid(thread_over->pid);

    /* 如果需要下一轮调度则主动调用schedule */
    if (need_schedule)
    {
        schedule();
        PANIC("thread_exit: should not be here\n");
    }
}

/* 比对任务的pid */
static bool pid_check(struct list_elem *pelem, int32_t pid)
{
    task_struct *pthread = elem2entry(task_struct, all_list_tag, pelem);
    if (pthread->pid == pid)
    {
        return true;
    }
    return false;
}

/* 根据pid找pcb,若找到则返回该pcb,否则返回NULL */
task_struct *pid2thread(int32_t pid)
{
    struct list_elem *pelem = list_traversal(&thread_all_list, pid_check, pid);
    if (pelem == NULL)
    {
        return NULL;
    }
    task_struct *thread = elem2entry(task_struct, all_list_tag, pelem);
    return thread;
}

void thread_init()
{
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);

    pid_pool_init();
    process_execute(init, "init");

    make_main_thread();
    idle_thread = thread_start("idle", 10, idle, NULL);
    put_str("thread_init done\n");
}