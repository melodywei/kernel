%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR


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

    ; -------------------------   加载kernel  ----------------------
    mov eax, KERNEL_START_SECTOR        ; kernel.bin所在的扇区号
    mov ebx, KERNEL_BIN_BASE_ADDR       ; 从磁盘读出后，写入到ebx指定的地址
    mov ecx, 200                ; 读入的扇区数

    call rd_disk_m_32

    
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

    jmp SELECTOR_CODE:enter_kernel     ;强制刷新流水线,更新gdt
enter_kernel:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    call kernel_init
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY_POINT                 ; 用地址0x1500访问测试，结果ok



;-----------------   将kernel.bin中的segment拷贝到编译的地址   -----------
kernel_init:
    xor eax, eax
    xor ebx, ebx     ;ebx记录程序头表地址
    xor ecx, ecx     ;cx记录程序头表中的program header数量
    xor edx, edx     ;dx 记录program header尺寸,即e_phentsize

    mov dx, [KERNEL_BIN_BASE_ADDR + 42]    ; 偏移文件42字节处的属性是e_phentsize,表示program header大小
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28]   ; 偏移文件开始部分28字节的地方是e_phoff,表示第1 个program header在文件中的偏移量
                      ; 其实该值是0x34,不过还是谨慎一点，这里来读取实际值
    add ebx, KERNEL_BIN_BASE_ADDR
    mov cx, [KERNEL_BIN_BASE_ADDR + 44]    ; 偏移文件开始部分44字节的地方是e_phnum,表示有几个program header
.each_segment:
    cmp byte [ebx + 0], PT_NULL        ; 若p_type等于 PT_NULL,说明此program header未使用。
    je .PTNULL

 
    ;为函数memcpy压入参数,参数是从右往左依然压入.函数原型类似于 memcpy(dst,src,size)
    push dword [ebx + 16]          ; program header中偏移16字节的地方是p_filesz,>压入函数memcpy的第三个参数:size
    mov eax, [ebx + 4]             ; 距程序头偏移量为4字节的位置是p_offset
    add eax, KERNEL_BIN_BASE_ADDR      ; 加上kernel.bin被加载到的物理地址,eax为该段的物理地址
    push eax               ; 压入函数memcpy的第二个参数:源地址
    push dword [ebx + 8]           ; 压入函数memcpy的第一个参数:目的地址,偏移程序头8字节的位置是p_vaddr，这就是目的地址
    call mem_cpy               ; 调用mem_cpy完成段复制
    add esp,12                 ; 清理栈中压入的三个参数
.PTNULL:
    add ebx, edx               ; edx为program header大小,即e_phentsize,在此ebx指>向下一个program header 
    loop .each_segment
    ret



;----------  逐字节拷贝 mem_cpy(dst,src,size) ------------
;输入:栈中三个参数(dst,src,size)
;输出:无
;---------------------------------------------------------
mem_cpy:
    cld
    push ebp
    mov ebp, esp
    push ecx        ; rep指令用到了ecx，但ecx对于外层段的循环还有用，故先入栈备份
    mov edi, [ebp + 8]      ; dst
    mov esi, [ebp + 12]     ; src
    mov ecx, [ebp + 16]     ; size
    rep movsb           ; 逐字节拷贝

   ;恢复环境
    pop ecx
    pop ebp
    ret


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



;-------------------------------------------------------------------------------
               ;功能:读取硬盘n个扇区
rd_disk_m_32:
;-------------------------------------------------------------------------------
    ; eax=LBA扇区号
    ; ebx=将数据写入的内存地址
    ; ecx=读入的扇区数
    mov esi,eax      ; 备份eax
    mov di,cx        ; 备份扇区数到di
;读写硬盘:
;第1步：设置要读取的扇区数
    mov dx,0x1f2
    mov al,cl
    out dx,al            ;读取的扇区数

    mov eax,esi      ;恢复ax
;第2步：将LBA地址存入0x1f3 ~ 0x1f6

    ;LBA地址7~0位写入端口0x1f3
    mov dx,0x1f3
    out dx,al

    ;LBA地址15~8位写入端口0x1f4
    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx, al
    
    ;LBA地址23~16位写入端口0x1f5
    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f      ;lba第24~27位
    or al,0xe0       ; 设置7～4位为1110,表示lba模式

    mov dx,0x1f6
    out dx,al

;第3步：向0x1f7端口写入读命令，0x20 
    mov dx,0x1f7
    mov al,0x20
    out dx,al

;;;;;;; 至此,硬盘控制器便从指定的lba地址(eax)处,读出连续的cx个扇区,下面检查硬盘>状态,不忙就能把这cx个扇区的数据读出来

;第4步：检测硬盘状态
  .not_ready:          ;测试0x1f7端口(status寄存器)的的BSY位
      ;同一端口,写时表示写入命令字,读时表示读入硬盘状态
    nop
    in al,dx
    and al,0x88      ;第4位为1表示硬盘控制器已准备好数据传输,第7位为1表示硬盘>忙
    cmp al,0x08
    jnz .not_ready       ;若未准备好,继续等。
;第5步：从0x1f0端口读数据
    mov ax, di       ;以下从硬盘端口读数据用insw指令更快捷,不过尽可能多的演示>命令使用,
               ;在此先用这种方法,在后面内容会用到insw和outsw等

    mov dx, 256      ;di为要读取的扇区数,一个扇区有512字节,每次读入一个字,共需di*512/2次,所以di*256
    mul dx
    mov cx, ax
    mov dx, 0x1f0
  .go_on_read:
    in ax,dx
    mov [ebx], ax
    add ebx, 2
             
    loop .go_on_read
    ret
