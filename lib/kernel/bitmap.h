#ifndef _LIB_KERNEL_BITMAP_H_
#define _LIB_KERNEL_BITMAP_H_

#include "../stdint.h"
#include "../../kernel/global.h"

#define BITMAP_MASK 1  // 通过按位与的方式判断位图中的相应位是否位1

// 
struct bitmap
{
    uint32_t btmp_bytes_len;
    uint8_t *bits;
};

// 初始化位图，将位图中的数据清0
void bitmap_init(struct bitmap *bitmp);

// 判断位图中的第bit_idx位是否为1, 如果是，返回true
bool bitmap_scan_test(struct bitmap *btmp, uint32_t bit_idx);

// 在位图中申请连续的cnt位个空间，成功 返回在位图中的起始下标(单位bit)
int bitmap_scan(struct bitmap *btmp, uint32_t cnt);

// 将位图中的bit_idx位 设置为value
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value);


#endif // !_LIB_KERNEL_BITMAP_H_