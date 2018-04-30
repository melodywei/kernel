#include "../lib/kernel/print.h"
#include "init.h"
#include "../thread/thread.h"
#include "interrupt.h"
#include "../device/console.h"
#include "../userprog/process.h"
#include "../userprog/syscall-init.h"
#include "../lib/user/syscall.h"

void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;
int prog_a_pid = 0, prog_b_pid = 0;

int main(void)
{
    put_str("I am kernel\n");
    init_all();

    
    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");

    intr_enable();
    console_put_str(" main pid: 0x");
    console_put_int(sys_getpid());
    console_put_char('\n');

    thread_start("k_thread_a", 31, k_thread_a, "argA ");
    thread_start("k_thread_b", 31, k_thread_b, "argB ");
    while (1)
        ;
    return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void *arg)
{
    char *para = arg;
    console_put_str(" thread a pid: 0x");
    console_put_int(sys_getpid());
    console_put_char('\n');

    console_put_str(" prog a pid: 0x");
    console_put_int(prog_a_pid);
    console_put_char('\n');
    while (1)
    {
    }
}

/* 在线程中运行的函数 */
void k_thread_b(void *arg)
{
    char *para = arg;
    console_put_str(" thread b pid: 0x");
    console_put_int(sys_getpid());
    console_put_char('\n');

    console_put_str(" prog b pid: 0x");
    console_put_int(prog_b_pid);
    console_put_char('\n');
    while (1)
    {
    }
}

/* 测试用户进程 */
void u_prog_a(void)
{
    prog_a_pid = getpid();
    while (1)
    {
       
    }
}

/* 测试用户进程 */
void u_prog_b(void)
{
    prog_b_pid = getpid();
    while (1)
    {
    }
}
