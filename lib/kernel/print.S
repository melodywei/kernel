;--------------打印功能--------
; 备份寄存器现场
; 获取光标位置
; 获取代打印的字符
; 判断是否需要滚屏
; 更新光标位置
; 恢复寄存器现场
;-----------------------------


TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x3 << 3) + TI_GDT + RPL0

section .data
put_int_buffer dq 0

[bits 32]
;--------put_char-------------
; 把栈中的1个字符写入光标所在处
;-----------------------------
section .text

;--------put_str-------
global put_str
put_str:
    push ebx
    push ecx
    xor ecx, ecx
    mov ebx, [esp + 12];获取待打印的字符串地址
.goon:
    mov cl, [ebx]
    cmp cl, 0
    jz .str_over
    push ecx
    call put_char
    add esp, 4
    inc ebx
    jmp .goon
.str_over:
    pop ecx
    pop ebx
    ret


;-------put_char--------
global put_char
put_char:
    pushad 
    mov ax, SELECTOR_VIDEO
    mov gs, ax

;获取光标位置
;--------------------

    ;先获取高8位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    in al, dx
    mov ah, al
    
    ;再获取低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx

    ;将光标存入bx
    mov bx, ax
    ;
    ;在栈中获取待打印的字符 pushad圧入4×8=32字节+主调函数返回地址=36
    mov ecx, [esp + 36]

    cmp cl, 0xd     ;CR=0xd  LF=0xa
    jz .is_carriage_return
    cmp cl, 0xa
    jz .is_line_feed

    cmp cl, 0x8   ;BS=0x8
    jz .is_backspace
    jmp .put_other


.is_backspace:
;删除前一个字符，将光标前移一位，然后将当前光标所在处填充0
    dec bx
    shl bx, 1

    mov byte [gs:bx], 0x20
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    jmp .set_cursor

.put_other:
    shl bx, 1
    mov [gs:bx], cl
    inc bx
    mov byte [gs:bx], 0x7
    shr bx, 1
    inc bx
    cmp bx, 2000
    jl .set_cursor


.is_line_feed:
.is_carriage_return:
;CR(\r)  LF(\n) 只需要将光标移到下一行行首
    xor dx, dx
    mov ax, bx
    mov si, 80

    div si
    sub bx, dx

.is_carriage_return_end:
    add bx, 80
    cmp bx, 2000
.is_line_feed_end:
    jl .set_cursor

.roll_screen:   ;超出屏幕大小时，开始滚屏，即将所有字符向上移动一行，将最后一行空出来
    cld

    mov ecx, 960    ;1920个字符， 1920×2=3840字节，一次4字节，共3840/4=960次
    mov esi, 0xc00b80a0 ;第一行行首
    mov edi, 0xc00b8000 ;第0行行首
    rep movsd

    mov ebx, 3840
    mov ecx, 80

.cls:
    mov word [gs:ebx], 0x0720
    add ebx, 2
    loop .cls
    mov bx, 1920

.set_cursor:
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al

    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al
.put_char_done:
    popad
    ret

    
;--------------------   将小端字节序的数字变成对应的ascii后，倒置   -----------------------
;输入：栈中参数为待打印的数字
;输出：在屏幕上打印16进制数字,并不会打印前缀0x,如打印10进制15时，只会直接打印f，不会是0xf
;------------------------------------------------------------------------------------------
global put_int
put_int:
    pushad
    mov ebp, esp 
    mov eax, [ebp+4*9]              ; call的返回地址占4字节+pushad的8个4字节
    mov edx, eax 
    mov edi, 7                          ; 指定在put_int_buffer中初始的偏移量
    mov ecx, 8                  ; 32位数字中,16进制数字的位数是8个
    mov ebx, put_int_buffer

;将32位数字按照16进制的形式从低位到高位逐个处理,共处理8个16进制数字
.16based_4bits:                ; 每4位二进制是16进制数字的1位,遍历每一位16进制数字
    and edx, 0x0F             ; 解析16进制数字的每一位。and与操作后,edx只有低4位有效
    cmp edx, 9                  ; 数字0～9和a~f需要分别处理成对应的字符
    jg .is_A2F 
    add edx, '0'                ; ascii码是8位大小。add求和操作后,edx低8位有效。
    jmp .store
.is_A2F:
    sub edx, 10                 ; A~F 减去10 所得到的差,再加上字符A的ascii码,便是A~F对应的ascii码
    add edx, 'A'

;将每一位数字转换成对应的字符后,按照类似“大端”的顺序存储到缓冲区put_int_buffer
;高位字符放在低地址,低位字符要放在高地址,这样和大端字节序类似,只不过咱们这里是字符序.
.store:
; 此时dl中应该是数字对应的字符的ascii码
    mov [ebx+edi], dl
    dec edi
    shr eax, 4
    mov edx, eax
    loop .16based_4bits

;现在put_int_buffer中已全是字符,打印之前,
;把高位连续的字符去掉,比如把字符000123变成123
.ready_to_print:
    inc edi                 ; 此时edi退减为-1(0xffffffff),加1使其为0
.skip_prefix_0:
    cmp edi,8                   ; 若已经比较第9个字符了，表示待打印的字符串为全0 
    je .full0 
;找出连续的0字符, edi做为非0的最高位字符的偏移
.go_on_skip:   
    mov cl, [put_int_buffer+edi]
    inc edi
    cmp cl, '0' 
    je .skip_prefix_0               ; 继续判断下一位字符是否为字符0(不是数字0)
    dec edi                 ;edi在上面的inc操作中指向了下一个字符,若当前字符不为'0',要恢复edi指向当前字符               
    jmp .put_each_num


.full0:
    mov cl,'0'                  ; 输入的数字为全0时，则只打印0
.put_each_num:
    push ecx                ; 此时cl中为可打印的字符
    call put_char
    add esp, 4
    inc edi                 ; 使edi指向下一个字符
    mov cl, [put_int_buffer+edi]        ; 获取下一个字符到cl寄存器
    cmp edi,8
    jl .put_each_num
    popad
    ret
