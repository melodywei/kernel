#ifndef _LIB_KERNEL_IO_H_
#define _LIB_KERNEL_IO_H_

#include "../stdint.h"

// 向端口port写入一字节数据
static inline void outb(uint16_t port, uint8_t data)
{
    // 对端口指定N表示0～255, d表示用dx存储端口号
    // %b0表示对应al， %w1表示对应dx
    asm volatile("out %b0, %w1"::"a"(data),"Nd"(port));
}

// 将addr处起始的word_cnt个字节写入端口port
static inline void outsw(uint16_t port, const void *addr, uint32_t word_cnt)
{
    // outsw是把ds:esi处的16位的内容写入port端口
    asm volatile("cld; rep outsw":"+S"(addr), "+c"(word_cnt):"d"(port));
}

// 将从端口port读入的一个字节返回
static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile("inb %w1, %b0":"=a"(data):"Nd"(port));
    return data;
}

// 将从端口port读入的word_cnt个字节写入addr
static inline void insw(uint16_t port, void *addr, uint32_t word_cnt)
{
    asm volatile("cld; rep insw":"+D"(addr), "+c"(word_cnt):"d"(port):"memory");
}

#endif //!_LIB_KERNEL_IO_H_