; 主引导程序
;---------------------

%include "boot.inc"

SECTION MBR vstart=0x7c00 ;程序开始的地址
    mov ax, cs            ;使用cs初始化其他的寄存器
    mov ds, ax            ;因为是通过jmp 0:0x7c00到的MBR开始地址
    mov es, ax            ;所以此时的cs为0,也就是用0初始化其他寄存器
    mov ss, ax            ;此类的寄存器不同通过立即数赋值，采用ax中转
    mov fs, ax
    mov sp, 0x7c00  ;初始化栈指针，sp也就是32位下的esp

;清屏利用0x10中断的0x6号功能
;------------------------
    mov ax, 0x600
    mov bx, 0x700
    mov cx, 0
    mov dx, 0x184f

    int 0x10

;获取光标位置
;---------------------
    mov ah, 3   ; 3号子功能获取光标位置
    mov bh, 1   ; bh寄存器存储带获取光标位置的页号,从0开始，此处填1可以看成将光标移动到最开始
    int 0x10

;打印字符串`
;------------------
    mov ax, message
    mov bp, ax

    mov cx, 6
    mov ax, 0x1301
    mov bx, 0x2

    int 0x10
    
    message db "My MBR"

    mov eax, LOADER_START_SECTOR    ;起始扇区的lba地址
    mov bx, LOADER_BASE_ADDR        ;写入的地址
    mov cx, 1                       ;读入的扇区数
    call rd_disk_m_16

    jmp LOADER_BASE_ADDR

;读取n个扇区
;---------------------
rd_disk_m_16:       ;eax=扇区号，cx=读入的扇区数，bx=将数据写入的内存地址
    mov esi, eax    ;备份eax和cx
    mov di, cx

;设置要读取的扇区数
    mov dx, 0x1f2
    mov al, al
    out dx, al
    mov eax, esi

;将lba地址存入0x1f3-0x1f6

    ;lba地址0-7位写入端口0x1f3
    mov dx, 0x1f3
    out dx, al

    ;lba地址8-15位写入端口0x1f4
    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    ;lba地址16-23位写入端口0x1f5
    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    
    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

;向0x1f7端口写入读命令
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al
.not_ready:
    nop
    in al, dx
    and al, 0x88 ;第4位为1表示硬盘控制器已经准备号数据传输，第7位为1表示硬盘忙
    cmp al, 0x08
    jnz .not_ready

;从0x1f0端口读数据
    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    mov dx, 0x1f0
.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret

    
    times 510-($-$$) db 0 
    db 0x55, 0xaa