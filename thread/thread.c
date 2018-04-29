#include "thread.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../lib/kernel/list.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"
#include "../kernel/memory.h"
#include "../lib/kernel/print.h"

task_struct *main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem *thread_tag;

extern void switch_to(task_struct *cur, task_struct *next);

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
  
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;     // thread_tag清空
    /* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
    thread_tag = list_pop(&thread_ready_list);
    task_struct* next = elem2entry(task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
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


void thread_init()
{
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);

    make_main_thread();
    put_str("thread_init done\n");
}