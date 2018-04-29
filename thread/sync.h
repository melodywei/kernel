#ifndef _THREAD_SYNC_H_
#define _THREAD_SYNC_H_

#include "../lib/kernel/list.h"
#include "../lib/stdint.h"
#include "thread.h"

/* 信号量结构 */
struct semaphore 
{
   uint8_t  value;
   struct   list waiters;
};

/* 锁结构 */
struct lock {
   task_struct* holder;        // 锁的持有者
   struct   semaphore semaphore;        // 用二元信号量实现锁
   uint32_t holder_repeat_nr;           // 锁的持有者重复申请锁的次数
};

void sema_init(struct semaphore* psema, uint8_t value); 
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_init(struct lock* plock);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif  // !_THREAD_SYNC_H_
