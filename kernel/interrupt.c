#include "interrupt.h"
#include "global.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"

#define PIC_M_CTRL 0x20        // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21        // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0        // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1        // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x21  // 目前支持的中断数

// 中断门描述副结构体
struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;
    uint8_t attribute;
    uint16_t func_offset_high_word;
};

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler func);
static struct gate_desc idt[IDT_DESC_CNT]; // idt是中断描述符表

extern intr_handler intr_entry_table[IDT_DESC_CNT];// 引用kernel.S中的中断处理函数入口数组


// 创建中断描述符表
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0xffff;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;
}

// 初始化可编程中断控制器8259A
static void pic_init()
{
    // 初始化主片
    outb(PIC_M_CTRL, 0x11);
    outb(PIC_M_DATA, 0x20);
    outb(PIC_M_DATA, 0x04);
    outb(PIC_M_DATA, 0x01);

    // 初始化从片
    outb(PIC_S_CTRL, 0x11);
    outb(PIC_S_DATA, 0x28);
    outb(PIC_S_DATA, 0x02);
    outb(PIC_S_DATA, 0x01);

    // 打开主片上的IR0, 也就是目前只接受时钟产生的中断
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    put_str("   pic_init done!\n");
}

// 初始化中断描述符表
static void idt_desc_init()
{
    for (int i = 0; i < IDT_DESC_CNT; ++i)
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("   idt_desc_init done\n");
}

void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init();
    pic_init();
    
    uint64_t idt_operand = (sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16));
    asm volatile("lidt %0"::"m"(idt_operand));
    put_str("idt_init done!\n");
}
