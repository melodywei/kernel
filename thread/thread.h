#ifndef _THREAD_THREAD_H_
#define _THREAD_THREAD_H_

#include "../lib/stdint.h"
#include "../lib/kernel/list.h"

typedef void thread_func(void*);

// 线程的状态
typedef enum tag_task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
}task_status;

// 中断栈
// 此结构用于中断发生时保护程序上下文环境
// 此栈放于内核栈中的固定位置，即内核栈所在页的最顶端
typedef struct tag_intr_stack
{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;

    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;
    void (*eip)();
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
}intr_stack;


// 线程栈
typedef struct tag_thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;


    void (*eip)(thread_func *func, void *func_arg);

    void *unused_retaddr;
    thread_func *function;
    void *func_arg;
}thread_stack;

// 线程pcb
typedef struct tag_task_struct
{
    uint32_t *self_kstack; // 内核线程的栈顶地址
    task_status status;    // 当前线程的状态
    char name[16];
    uint8_t priority;
    uint8_t ticks;  // 线程执行的时间

    uint32_t elapsed_ticks;  // 线程已经执行的时间

    struct list_elem general_tag;

    struct list_elem all_list_tag;
    uint32_t *pgdir;

   
    uint32_t stack_magic;   // 栈的边界标记，用来检测栈溢出
}task_struct;

void thread_create(task_struct* pthread, thread_func function, void* func_arg);
void init_thread(task_struct* pthread, char* name, int prio);
task_struct* thread_start(char* name, int prio, thread_func function, void*  func_arg);

task_struct *running_thread();
void schedule();

void thread_init();

void thread_unblock(task_struct* pthread);
void thread_block(task_status stat);

#endif //!_THREAD_THREAD_H_