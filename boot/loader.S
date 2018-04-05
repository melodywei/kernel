%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start


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
times 60 dq 0

SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0     ; 相当于(CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0     ; 同上
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0    ; 同上 

gdt_ptr dw GDT_LIMIT    ;gdt的前2字节是段界限，后4字节是段基址
        dd GDT_BASE
loadermsg db '2 loader in real.'

loader_start:
    mov sp, LOADER_BASE_ADDR
    mov bp, loadermsg
    mov cx, 17
    mov ax, 0x1301
    mov bx, 0x001f
    mov dx, 0x1800
    int 0x10

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
