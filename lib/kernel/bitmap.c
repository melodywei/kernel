#include "bitmap.h"
#include "../stdint.h"
#include "../string.h"
#include "../../kernel/interrupt.h"
#include "print.h"
#include "../../kernel/debug.h"

// 初始化位图，将位图中的数据清0
void bitmap_init(struct bitmap *btmp)
{
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

// 判断位图中的第bit_idx位是否为1, 如果是，返回true
bool bitmap_scan_test(struct bitmap *btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8; // 位图中bit_idx所在的byte索引
    uint32_t bit_odd = bit_idx % 8; // 该bit在 1byte中的偏移量

    return btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd);
}

// 在位图中申请连续的cnt位个空间，成功返回在位图中的起始下标
int bitmap_scan(struct bitmap *btmp, uint32_t cnt)
{
    uint32_t idx_byte = 0;

    while((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len))
    {// 判断该字节是否全部为1, 且位图大小必须 小于 要申请的空间
        // 代表该字节已全部占用，继续向下字节查找
        ++idx_byte;
    }

    ASSERT(idx_byte < btmp->btmp_bytes_len);

    // 内存池中找不到可用空间
    if (idx_byte == btmp->btmp_bytes_len)
    {
        return -1;
    }

    // 在idx_byte中有空闲位后，对该byte进行逐bit比对，直到有连续的bits=cnt
    int idx_bit = 0;
    while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte])
    {// 找到idx_byte中为0的bit所在的位置
        ++idx_bit;
    }

    int bit_idx_start = idx_byte * 8 + idx_bit; // 空闲位在位图中的bit偏移量

    if(cnt == 1)
    {
        return bit_idx_start;
    }

    // 位图中还剩余的bits数
    uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);

    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1;  // 记录找到的空闲位数量

    bit_idx_start = -1;
    // 在剩余的空间中继续查找，直到有连续的bits=cnt
    while (bit_left-- > 0)
    {
        if(!bitmap_scan_test(btmp, next_bit))
        {
            ++count;
        }
        else
        {
            count = 0;
        }

        if(count == cnt)
        {
            bit_idx_start = next_bit - cnt + 1;
        }

        ++next_bit;
    }
    return bit_idx_start;
}

// 将位图中的bit_idx位 设置为value
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value)
{
    ASSERT(value == 0 || value == 1);

    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;

    if(value)
    {
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    }
    else
    {
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}