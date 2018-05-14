#ifndef _LIB_USER_SYSCALL_H_
#define _LIB_USER_SYSCALL_H_


#include "../stdint.h"

enum SYSCALL_NR 
{
   SYS_GETPID,
   SYS_WRITE,
   SYS_MALLOC,
   SYS_FREE,
   SYS_OPEN,
   SYS_CLOSE,
   SYS_READ,
   SYS_LSEEK,
   SYS_UNLINK
};

uint32_t getpid(void);
uint32_t write(int32_t fd, const void *buf, uint32_t count);
void *malloc(uint32_t size);
void free(void *ptr);
int open(const char *pathname, int flags);
int close(int fd);
int32_t read(int32_t fd, void *buf, uint32_t count);
int32_t lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t unlink(const char *pathname);
#endif

