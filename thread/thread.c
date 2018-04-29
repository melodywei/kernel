#include "thread.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"

static void kernel_thread(thread_func *function, void *func_arg)
{
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
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;

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
    asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret"  : : "g" (thread->self_kstack) : "memory");     
    return thread;  
  }