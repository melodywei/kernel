#include "syscall-init.h"
#include "../lib/user/syscall.h"
#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../lib/string.h"
#include "../kernel/memory.h"
#include "../fs/fs.h"

#define syscall_nr 32
typedef void *syscall;
syscall syscall_table[syscall_nr];

/* 返回当前任务的pid */
uint32_t sys_getpid(void)
{
    return running_thread()->pid;
}

uint32_t sys_write(char *str)
{
    console_put_str(str);
    return strlen(str);
}

/* 初始化系统调用 */
void syscall_init(void)
{
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_MALLOC] = sys_malloc;
    syscall_table[SYS_FREE] = sys_free;
    syscall_table[SYS_OPEN] = sys_open;
    syscall_table[SYS_CLOSE] = sys_close;
    put_str("syscall_init done\n");
}
