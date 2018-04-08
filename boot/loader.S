%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR


; 构建全局描述符表，并填充段描述符，段描述符的大小为8字节，在这里将其分为低4字节与高4字节来定义
; dd=define double-word，为4字节
;--------------------------------------------------------

; gdt的起始地址为GDT_BASE的地址，且gdt的第0个描述符不可用，所以将其直接定义为0
GDT_BASE: dd 0x00000000
            dd 0x00000000

; 代码段
CODE_DESC: dd 0x0000ffff
            dd DESC_CODE_HIGH4

; 数据段和栈段
DATA_STACK_DESC: dd 0x0000ffff
                 dd DESC_DATA_HIGH4

; 显存段描述符
VIDEO_DESC: dd 0x80000007
            dd DESC_VIDEO_HIGH4

GDT_SIZE equ   $-GDT_BASE
GDT_LIMIT equ GDT_SIZE - 1
times 60 dq 0 ;dq表示8字节的数据，在这里预留出60个8字节的空间。以便将来扩充

SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0     ; 相当于(CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0     ; 同上
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0    ; 同上 

;total_mem_bytes 存放获取到的总内存，该变量的地址为  0xb00 = 0x900 + 60 * 8 + 8 * 4;
total_mem_bytes dd 0

gdt_ptr dw GDT_LIMIT    ;gdt的前2字节是段界限，后4字节是段基址
        dd GDT_BASE

ards_buf times 244 db 0
ards_nr dw 0

loader_start:
;int 0x15 eax=0xe820 edx=0x534d4150
;--------------------------------------
    xor ebx, ebx    ;将ebx清0
    mov edx, 0x534d4150 
    mov di, ards_buf  ;ards结构缓冲区
  .e820_mem_get_loop:
    mov eax, 0xe820
    mov ecx, 20   ;ards地址范围描述符结构大小位20字节
    int 0x15
    
    add di, cx  ;使di增加20字节指向缓冲区中新的ards结构位置
    inc word [ards_nr] ;记录ards数量
    cmp ebx, 0  ;如果ebx为0且cf位不为1,说明adrs全部返回
    jnz .e820_mem_get_loop

    ;在所有ards结构中找出（base_addr_low + length_low)的最大值，即为内存的容量
    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor edx, edx
  .find_max_mem_area:
    mov eax, [ebx]  ;base_addr_low
    add eax, [ebx + 8] ;length_low
    add ebx, 20
    cmp edx, eax
    jge .next_ards
    mov edx, eax
  .next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

  .mem_get_ok:
    mov [total_mem_bytes], edx





;---------------------------
;准备进入保护模式
;1. 打开A20
;2. 加载gdt
;3. 将cr0的PE位置1
;---------------------------


;-------打开A20--------
    in al, 0x92
    or al, 0000_0010b
    out 0x92, al

;-------加载gdt-------
    lgdt [gdt_ptr]

;------cr0第0位置1-----
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    jmp SELECTOR_CODE:p_mode_start


[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    
    mov byte [gs:160], 'P'

    jmp $
