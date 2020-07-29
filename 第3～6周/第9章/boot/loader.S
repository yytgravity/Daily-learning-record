    %include "boot.inc"
    section loader vstart=LLOADER_BASE_ADDR
    
    GDT_BASE:   dd  0x00000000
                dd  0x00000000

    CODE_DESC:  dd  0x0000FFFF
                dd  DESC_CODE_HIGH4

    DATA_STACK_DESC:  dd  0x0000FFFF
                      dd  DESC_DATA_HIGH4
    
    VIDEO_DESC:  dd  0x80000007           ;limit=(0xbffff-0xb8000)/4k=0x7
                 dd  DESC_VIDEO_HIGH4     ;此时dpl为0
  
    GDT_SIZE  equ  $ - GDT_BASE
    GDT_LIMIT  equ  GDT_SIZE - 1
    times 60 dq 0
    SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
    SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO  equ (0x0003<<3) + TI_GDT + RPL0

    ; total_mem_bytes用于保存内存容量,以字节为单位,此位置比较好记。
    ; 当前偏移loader.bin文件头0x200字节,loader.bin的加载地址是0x900,
    ; 故total_mem_bytes内存中的地址是0xb00.将来在内核中咱们会引用此地址
    total_mem_bytes dd 0
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ;以下是定义gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址
    gdt_ptr  dw  GDT_LIMIT
             dd  GDT_BASE

    ;人工对齐:total_mem_bytes4字节+gdt_ptr6字节+ards_buf244字节+ards_nr2,共256字节
    ards_buf times 244 db 0
    ards_nr dw 0              ;用于记录ards结构体数量

    loader_start:

;-------  int 15h eax = 0000E820h ,edx = 534D4150h ('SMAP') 获取内存布局  -------
    
    xor ebx,ebx
    mov edx,0x534d4150
    mov di,ards_buf
 .e820_mem_get_loop:
    mov eax,0x0000e820     ;执行int 0x15后,eax值变为0x534d4150,所以每次执行int前都要更新为子功能号。
    mov ecx,20
    int 0x15
    jc .e820_failed_so_try_e801 ;若cf位为1则有错误发生，尝试0xe801子功能
    add di,cx ;若cf位为1则有错误发生，尝试0xe801子功能
    inc word [ards_nr] ;记录ARDS数量
    cmp ebx,0
    jnz .e820_mem_get_loop

;在所有ards结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
    mov cx,[ards_nr]
    mov ebx,ards_buf
    xor edx,edx
 .find_max_mem_area:
    mov eax,[ebx]
    add eax,[ebx+8]
    add ebx,20
    cmp edx,eax
    jge .next_ards
    mov edx,eax
 .next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

;------ int 15h ax = E801h 获取内存大小，最大支持 4G ------
; 返回后, ax cx 值一样,以 KB 为单位，bx dx 值一样，以 64KB 为单位
; 在ax和cx寄存器中为低16MB，在bx和dx寄存器中为16MB到4GB

 .e820_failed_so_try_e801:
    mov ax,0xe801
    int 0x15
    jc .e801_failed_so_try88   ;若当前e801方法失败,就尝试0x88方法

;1 先算出低15M的内存,ax和cx中是以KB为单位的内存数量,将其转换为以byte为单位
    mov cx,0x400
    mul cx
    shl edx,16
    and eax,0x0000FFFF
    or edx,eax
    add edx,0x100000
    mov esi,edx

;2 再将16MB以上的内存转换为byte为单位,寄存器bx和dx中是以64KB为单位的内存数量

    xor eax,eax
    mov ax,bx
    mov ecx,0x10000
    mul ecx
    add esi,eax
    mov edx,esi
    jmp .mem_get_ok

;-----------------  int 15h ah = 0x88 获取内存大小,只能获取64M之内  ----------

 .e801_failed_so_try88:
    ;int 15后，ax存入的是以kb为单位的内存容量
    mov ah,0x88
    int 0x15
    jc .error_hlt
    and eax,0x0000FFFF
    
;16位乘法，被乘数是ax,积为32位.积的高16位在dx中，积的低16位在ax中
    mov cx,0x400
    mul cx
    shl edx,16
    or edx,eax
    add edx,0x100000   ;0x88子功能只会返回1MB以上的内存,故实际内存大小要加上1MB

 .mem_get_ok:
    mov [total_mem_bytes],edx


;-----------------   准备进入保护模式   -------------------
;1 打开A20
;2 加载gdt
;3 将cr0的pe位置1

    ;-----------------  打开A20  ----------------

    in al,0x92
    or al,0000_0010B
    out 0x92,al

    ;-----------------  加载GDT  ----------------

    lgdt [gdt_ptr]


    ;-----------------  cr0第0位置1  ----------------
    mov eax,cr0
    or eax,0x00000001
    mov cr0,eax

    jmp dword SELECTOR_CODE:p_mode_start

 .error_hlt
    hlt

 [bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp,LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    
; -------------------------   加载kernel  ----------------------
    mov eax,KERNEL_START_SECTOR  ; kernel.bin所在的扇区号
    mov ebx,KERNEL_BIN_BASE_ADDR ; 从磁盘读出后，写入到ebx指定的地址
    mov exc,200 ; 读入的扇区数

    call rd_disk_m_32

    call setup_page

    ;要将描述符表地址及偏移量写入内存gdt_ptr,一会用新地址重新加载
    sgdt [gdt_ptr]          ; 存储到原来gdt所有的位置

    mov ebx,[gdt_ptr + 2]
    or dword [ebx + 0x18 + 4],0xc0000000
    
    add dword [gdt_ptr + 2],0xc0000000

    mov eax,PAGE_DIR_TABLE_POS
    mov cr3,eax

    mov eax,cr0
    or eax,0x80000000
    mov cr0,eax

    lgdt [gdt_ptr]

    jmp SELECTOR_CODE:enter_kernel

 enter_kernel:
    call kernel_init
    mov esp,0xc009f000
    jmp KERNEL_ENTRY_POINT


;-----------------   将kernel.bin中的segment拷贝到编译的地址   -----------
 kernel_init:
    xor eax,eax
    xor ebx,ebx     ;ebx记录程序头表地址
    xor ecx,ecx     ;cx记录程序头表中的program header数量
    xor edx,edx     ;dx 记录program header尺寸,即e_phentsize

   mov dx, [KERNEL_BIN_BASE_ADDR + 42]      ; 偏移文件42字节处的属性是e_phentsize,表示program header大小
   mov ebx, [KERNEL_BIN_BASE_ADDR + 28]   ; 偏移文件开始部分28字节的地方是e_phoff,表示第1 个program header在文件中的偏移量
                      ; 其实该值是0x34,不过还是谨慎一点，这里来读取实际值
   add ebx, KERNEL_BIN_BASE_ADDR
   mov cx, [KERNEL_BIN_BASE_ADDR + 44]    ; 偏移文件开始部分44字节的地方是e_phnum,表示有几个program header
.each_segment:
   cmp byte [ebx + 0], PT_NULL          ; 若p_type等于 PT_NULL,说明此program header未使用。
   je .PTNULL


;为函数memcpy压入参数,参数是从右往左依然压入.函数原型类似于 memcpy(dst,src,size)

    push dword [ebx + 16]
    mov eax,[ebx + 4]
    add eax,KERNEL_BIN_BASE_ADDR
    push eax
    push dword [ebx + 8]
    call mem_cpy
    add esp,12
.PTNULL:
    add ebx,edx
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
   push ecx           ; rep指令用到了ecx，但ecx对于外层段的循环还有用，故先入栈备份
   mov edi, [ebp + 8]       ; dst
   mov esi, [ebp + 12]       ; src
   mov ecx, [ebp + 16]       ; size
   rep movsb           ; 逐字节拷贝

   ;恢复环境
   pop ecx
   pop ebp
   ret

;-------------   创建页目录及页表   ---------------

 setup_page:

    mov ecx,4096
    mov esi,0

 .clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi],0
    inc esi
    loop .clear_page_dir

;开始创建页目录项（PDE）
 .create_pde:
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000
    mov ebx,eax

;   下面将页目录项0和0xc00都存为第一个页表的地址，
;   一个页表可表示4MB内存,这样0xc03fffff以下的地址和0x003fffff以下的地址都指向相同的页表，
;   这是为将地址映射为内核地址做准备

    or eax,PG_US_U | PG_RW_W | PG_P
    mov [PAGE_DIR_TABLE_POS + 0X0], eax
    mov [PAGE_DIR_TABLE_POS + 0Xc00],eax

    sub eax,0x1000
    mov [PAGE_DIR_TABLE_POS + 4092],eax

;下面创建页表项（pte）
    mov ecx,256
    mov esi,0
    mov edx, PG_US_U | PG_RW_W | PG_P
 .create_pte:
    mov [ebx+esi*4],edx
    add edx,4096
    inc esi
    loop .create_pte


;创建内核其他页表PDE
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000
    or eax, PG_US_U | PG_RW_W | PG_P
    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254
    mov esi,769

 .create_kernel_pde:
    mov [ebx+esi*4],eax
    inc esi
    add eax,0x1000
    loop .create_kernel_pde
    ret



;-------------------------------------------------------------------------------
               ;功能:读取硬盘n个扇区
 rd_disk_m_32:
;-------------------------------------------------------------------------------
                             ; eax=LBA扇区号
                             ; ebx=将数据写入的内存地址
                             ; ecx=读入的扇区数
      mov esi,eax       ; 备份eax
      mov di,cx           ; 备份扇区数到di
;读写硬盘:
;第1步：设置要读取的扇区数
      mov dx,0x1f2
      mov al,cl
      out dx,al            ;读取的扇区数

      mov eax,esi       ;恢复ax

;第2步：将LBA地址存入0x1f3 ~ 0x1f6

      ;LBA地址7~0位写入端口0x1f3
      mov dx,0x1f3
      out dx,al

      ;LBA地址15~8位写入端口0x1f4
      mov cl,8
      shr eax,cl
      mov dx,0x1f4
      out dx,al

      ;LBA地址23~16位写入端口0x1f5
      shr eax,cl
      mov dx,0x1f5
      out dx,al

      shr eax,cl
      and al,0x0f       ;lba第24~27位
      or al,0xe0       ; 设置7～4位为1110,表示lba模式
      mov dx,0x1f6
      out dx,al

;第3步：向0x1f7端口写入读命令，0x20
      mov dx,0x1f7
      mov al,0x20
      out dx,al

;;;;;;; 至此,硬盘控制器便从指定的lba地址(eax)处,读出连续的cx个扇区,下面检查硬盘状态,不忙就能把这cx个扇区的数据读出来

;第4步：检测硬盘状态
 .not_ready:           ;测试0x1f7端口(status寄存器)的的BSY位
      ;同一端口,写时表示写入命令字,读时表示读入硬盘状态
      nop
      in al,dx
      and al,0x88       ;第4位为1表示硬盘控制器已准备好数据传输,第7位为1表示硬盘忙
      cmp al,0x08
      jnz .not_ready       ;若未准备好,继续等。

;第5步：从0x1f0端口读数据
      mov ax, di       ;以下从硬盘端口读数据用insw指令更快捷,不过尽可能多的演示命令使用,
               ;在此先用这种方法,在后面内容会用到insw和outsw等

      mov dx, 256       ;di为要读取的扇区数,一个扇区有512字节,每次读入一个字,共需di*512/2次,所以di*256
      mul dx
      mov cx, ax
      mov dx, 0x1f0
 .go_on_read:
      in ax,dx
      mov [ebx], ax
      add ebx, 2

      loop .go_on_read
      ret

