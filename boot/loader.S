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
    jc .e820_failed_so_try_e801
    
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

  .e820_failed_so_try_e801:
    mov ax, 0xe801
    int 0x15
    jc .e801_failed_so_tyy88
    
    ;先算出低15MB的内存
    mov cx, 0x400 ;将获取到的低15M内存乘1024转化成byte
    mul cx
    shl edx, 16
    and eax, 0xffff ;只取低16位，防止乘法溢出
    or edx, eax
    add eax, 0x100000
    mov esi, edx

    ;再将16MB以上的空间转化成byte为单位
    xor eax, eax
    mov ax, bx
    mov ecx, 0x10000 ;32位下默认被乘数是eax，将获取到的内存乘以64KB转换成byte
    mul ecx
    add esi, eax
    mov edx, esi 
    jmp .mem_get_ok

  .e801_failed_so_tyy88:
    mov ah, 0x88
    int 0x15
    jc .error_hlt
    and eax, 0xffff
    mov cx, 0x400
    mul cx
    shl edx, 16
    or edx, eax
    add edx, 0x100000


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


.error_hlt:
    hlt

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    
    call setup_page

    ;要将描述符表地址及偏移量写入内存gdt_ptr,一会用新地址重新加载
    sgdt [gdt_ptr]         

    ;将gdt描述符中视频段描述符中的段基址+0xc0000000
    mov ebx, [gdt_ptr + 2]

    ;显存段是第3个段描述符,每个描述符是8字节,故0x18。;段描述符的高4字节的最高位是段基址的31~24位
    or dword [ebx + 0x18 + 4], 0xc0000000      
                          
    ;将gdt的基址加上0xc0000000使其成为内核所在的高地址
    add dword [gdt_ptr + 2], 0xc0000000

    ; 将栈指针同样映射到内核地址
    add esp, 0xc0000000       

    ; 把页目录地址赋给cr3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    ; 打开cr0的pg位(第31位)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
 
    ;在开启分页后,用gdt新的地址重新加载
    lgdt [gdt_ptr]             ; 重新加载

    mov byte [gs:160], 'V' 

    jmp $


;----------创建页目录及页表----------
setup_page:
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

.create_pde:                     
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000               ; 此时eax为第一个页表的位置及属性
    mov ebx, eax                  ; 此处为ebx赋值，是为.create_pte做准备，ebx为基址。

;   下面将页目录项0和0xc00都存为第一个页表的地址，
;   一个页表可表示4MB内存,这样0xc03fffff以下的地址和0x003fffff以下的地址都指向相同的页表，
;   这是为将地址映射为内核地址做准备
    or eax, PG_US_U | PG_RW_W | PG_P      ; 页目录项的属性RW和P位为1,US为1,表示用户属性,所有特权级别都可以访问.
    mov [PAGE_DIR_TABLE_POS + 0x0], eax       ; 第1个目录项,在页目录表中的第1个目录项写入第一个>页表的位置(0x101000)及属性(7)
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax     ; 一个页表项占用4字节,0xc00表示第768个页表占用的目录项,0xc00以上的目录项用于内核空间,
                             ; 也就是页表的0xc0000000~0xffffffff共计1G属于内核,0x0~0xbfffffff共计3G属于用户进程.
    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_POS + 4092], eax      ; 使最后一个目录项指向页目录表自己的地址

;下面创建页表项(PTE)
    mov ecx, 256                  ; 1M低端内存 / 每页大小4k = 256
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P         ; 属性为7,US=1,RW=1,P=1
.create_pte:                     
    mov [ebx+esi*4],edx               ; 此时的ebx已经在上面通过eax赋值为0x101000,也就是第一个页表的地址 
    add edx,4096
    inc esi 
    loop .create_pte
;创建内核其它页表的PDE
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000           ; 此时eax为第二个页表的位置
    or eax, PG_US_U | PG_RW_W | PG_P  ; 页目录项的属性US,RW和P位都为1
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254              ; 范围为第769~1022的所有目录项数量
    mov esi, 769
.create_kernel_pde:
    mov [ebx+esi*4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde
    ret


