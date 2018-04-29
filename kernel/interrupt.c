#include "interrupt.h"
#include "global.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"

#define PIC_M_CTRL 0x20        // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21        // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0        // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1        // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x21  // 目前支持的中断数

#define EFLAGS_IF 0x200     // eflags寄存器中的if位为1,IF位是第9位
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl;popl %0":"=g"(EFLAG_VAR))

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

char *intr_name[IDT_DESC_CNT]; // 保存异常名

// 保存中断处理程序的入口，里面存放函数指针
intr_handler idt_table[IDT_DESC_CNT];

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

// 通用的中断处理函数，一般用在异常出现时的处理
static void general_intr_handler(uint8_t vec_nr)
{
    if(vec_nr == 0x27 || vec_nr == 0x2f)
    {
        // 0x27是产生的为中断，无需处理
        // 0x2f是最后一个引脚，保留项
        return;
    }

    /* 将光标置为0,从屏幕左上角清出一片打印异常信息的区域,方便阅读 */
    set_cursor(0);
    int cursor_pos = 0;
    while(cursor_pos < 320)
    {
        put_char(' ');
        cursor_pos++;
    }
  
    set_cursor(0);    // 重置光标为屏幕左上角
    put_str("!!!!!!!      excetion message begin  !!!!!!!!\n");

    set_cursor(88);  // 从第2行第8个字符开始打印
    put_str(intr_name[vec_nr]);
    if (vec_nr == 14) 
    {    // 若为Pagefault,将缺失的地址打印出来并悬停
        int page_fault_vaddr = 0;
        asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));   // cr2是存放造成page_fault的地址
        put_str("\npage fault addr is ");
        put_int(page_fault_vaddr);
    }
    put_str("\n!!!!!!!      excetion message end    !!!!!!!!\n");

    // 能进入中断处理程序就表示已经处在关中断情况下,
    // 不会出现调度进程的情况。故下面的死循环不会再被中断。
    while(1);
}

// 完成一般中断处理函数注册及异常名称注册
static void exception_init()
{
    for (int i = 0; i < IDT_DESC_CNT; ++i)
    {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";

}

// 通过EFLAGS中的if位获取当前中断状态
enum intr_status intr_get_status()
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

// 开中断，返回中断前的状态
enum intr_status intr_enable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        return old_status;
    }

    old_status = INTR_OFF;
    asm volatile("sti"); // 开中断，sti指令将if位置1
    return old_status;
}

// 关中断，返回中断前的状态
enum intr_status intr_disable()
{
    enum intr_status old_status;
    if(INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        asm volatile("cli" ::: "memory"); //关中断，cli指令将if位置0
        return old_status;
    }
    
    old_status = INTR_OFF;
    return old_status;
}

// 设置中断状态
enum intr_status intr_set_status(enum intr_status status)
{
    return status & INTR_ON ? intr_enable() : intr_disable();
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

void register_handler(uint8_t vec_no, intr_handler function)
{
    idt_table[vec_no] = function;
}

void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();
    
    uint64_t idt_operand = (sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16));
    asm volatile("lidt %0"::"m"(idt_operand));
    put_str("idt_init done!\n");
}
