
%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR				; LOADER_BASE_ADDR = 0x900


jmp loader_start									; 跳到loader的入口									


;------------------------- 构建gdt及其内部的描述符 -----------------------
GDT_BASE: 
		dd	0x00000000 
	    dd	0x00000000
; 代码段描述符
CODE_DESC:  
		dd	0x0000FFFF 
	    dd  DESC_CODE_HIGH4
; 数据段和栈段描述符，数据段和栈段共用一个描述符
DATA_STACK_DESC:  
		dd  0x0000FFFF
		dd  DESC_DATA_HIGH4
; 显存段描述符
VIDEO_DESC: 
		dd  0x80000007	       		; limit=(0xbffff-0xb8000)/4k=0x7
	    dd  DESC_VIDEO_HIGH4  		; 此时dpl为0
	
GDT_SIZE   equ   $ - GDT_BASE
GDT_LIMIT   equ   GDT_SIZE -	1 
times 60 dq 0					 ; 此处预留60个描述符的slot

；代码段选择子	
SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0         	; 相当于(CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0
; 数据段选择子
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0	 		; 同上
; 显存段选择子
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0	 		; 同上 

;以下是定义gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址
gdt_ptr  
		dw  GDT_LIMIT 						; gdt界限
	    dd  GDT_BASE						; gdt起始地址

		
loadermsg db '2 loader in real.'


; loader的入口
loader_start:



; -------------------------- 获取内存大小 ----------------------------------
; 省略，不用看代码
; --------------------------------------------------------------------------
   

   
;----------------------------------------   准备进入保护模式   ------------------------------------------
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
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

	jmp  SELECTOR_CODE:p_mode_start	     			; 刷新流水线，避免分支预测的影响,这种cpu优化策略，最怕jmp跳转，
													; 此时cs为SELECTOR_CODE
													; 这将导致之前做的预测失效，从而起到了刷新的作用。
.error_hlt:		      ;出错则挂起
	hlt

[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA							
	mov ds, ax										; ds为SELECTOR_DATA
	mov es, ax										; es为SELECTOR_DATA
	mov ss, ax										; ss为SELECTOR_DATA
	mov esp,LOADER_STACK_TOP						; esp为LOADER_STACK_TOP，即0x900
	mov ax, SELECTOR_VIDEO						
	mov gs, ax										; gs为SELECTOR_VIDEO
	
	
;-------------------- 加载内核 -------------------------------
mov eax, KERNEL_START_SECTOR		; kernel.bin所在的扇区号，即0x9
mov ebx, KERNEL_BIN_BASE_ADDR		; 从磁盘读出后，写入到ebx指定的地址,即0x70000

mov ecx, 200						; 读入的扇区数
call rd_disk_m_32
	
   
; 创建页目录及页表并初始化页内存位图
call setup_page
   
;要将描述符表地址及偏移量写入内存gdt_ptr,一会用新地址重新加载
sgdt [gdt_ptr]	      							; 将GDT的起始地址和偏移量dump出来，存储到原来gdt所有的位置
   
;将gdt描述符中视频段描述符中的段基址+0xc0000000
mov ebx, [gdt_ptr + 2]							; 此时ebx就是gdt的起始地址  
or dword [ebx + 0x18 + 4], 0xc0000000      		;视频段是第3个段描述符,每个描述符是8字节,故0x18。
												;段描述符的高4字节的最高位是段基址的31~24位
	
add dword [gdt_ptr + 2], 0xc0000000				; 将gdt的基址加上0xc0000000使其成为内核所在的高地址
add esp, 0xc0000000        						; 将栈指针同样映射到内核地址，此时esp = 0xc0000900										

; 把页目录地址赋给cr3
mov eax, PAGE_DIR_TABLE_POS
mov cr3, eax
	
; 打开cr0的pg位(第31位)
mov eax, cr0
or eax, 0x80000000
mov cr0, eax
	
;在开启分页后,用gdt新的地址重新加载
lgdt [gdt_ptr]             ; 重新加载

;;;;;;;;;;;;;;;;;;;;;;;;;;;;  此时不刷新流水线也没问题  ;;;;;;;;;;;;;;;;;;;;;;;;
;由于一直处在32位下,原则上不需要强制刷新,经过实际测试没有以下这两句也没问题.
;但以防万一，还是加上啦，免得将来出来莫句奇妙的问题.
   jmp SELECTOR_CODE:enter_kernel	  ;强制刷新流水线,更新gdt
enter_kernel:    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	call kernel_init
	mov eap, 0xc009f000
	jmp KERNEL_ENTRY_POINT



   

;-----------------   将kernel.bin中的segment拷贝到编译的地址   -----------
kernel_init:
	xor eax, eax
	xor ebx, ebx					; ebx记录程序头表地址
	xor ecx, ecx					; cx记录程序头表中program header数量
	xor edx, edx					; dx记录program header尺寸，即e_phentsize
	
	mov dx, [KERNEL_BIN_BASE_ADDR + 42]		; 偏移文件42字节处的属性是e_phentsize，表示program header大小
	mov ebx, [KERNEL_BIN_BASE_ADDR + 28]	; 偏移文件开始28字节开始的地方是e_phoff，表示第一个program header在文件中的偏移量
	
	add ebx, KERNEL_BIN_BASE_ADDR			; 此时ebx作为程序头表的基址
	mov cx, [KERNEL_BIN_BASE_ADDR + 44]		; 偏移文件开始部分44字节的地方是e_phnum, 表示有几个program header
	
.each_segment:
	cmp byte [ebx + 0], PT_NULL				; 若pt_type等于PT_NULL, 说明此program header未使用
	je .PTNULL
	
	; 为函数memcpy压入参数，参数是从右往左依次压入，函数原型类似于memcpy(dst, src, size)
	push dword [ebx + 16]					; program header中偏移16字节的地方是p_filesz; 压入函数memcpy的第三个参数：size
	mov eax, [ebx + 4]						; 距程序头偏移量为4字节的位置是p_offset
	add eax, KERNEL_BIN_BASE_ADDR			; eax为该段的物理地址
	push eax								; 压入函数memcpy的第二个参数：源地址
	push dword [ebx + 8]					; 压入函数memcpy的第一个参数，目的地址，即p_vaddr，这就是目的地址
	
	call mem_cpy							; 调用mem_cpy完成段复制
	add esp, 12								; 清理栈找那个压入的三个参数
.PTNULL:
	add ebx, edx							; edx为program header大小，即e_phentsize
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
	push ecx				; rep指令用到了ecx, 但ecx对于外层段的循环还有用，故先入栈备份
	
	mov edi, [ebp + 8]		; dst
	mov esi, [ebp + 12]		; src
	mov ecx, [ebp + 16]		; size
	rep movsb				; 逐字节拷贝
	
	; 恢复环境
	pop ecx
	pop ebp
	ret




;-------------   创建页目录及页表   ---------------
setup_page:
	;先把页目录占用的空间逐字节清0
	mov ecx, 4096				; 页目录占4KB = 4096B
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir
	
;开始创建页目录项(PDE)
.create_pde:				     			; 创建Page Directory Entry
	mov eax, PAGE_DIR_TABLE_POS				; PAGE_DIR_TABLE_POS = 0x100000		
	add eax, 0x1000 			     		; 此时eax为第0个页表的位置，即0x101000,0x100000~0x101000是页目录表的位置
	mov ebx, eax				     		; 此处为ebx赋值，是为.create_pte做准备，ebx为基址。

;创建第一个页目录项
	or eax, PG_US_U | PG_RW_W | PG_P		;增加页目录项的属性
    mov [PAGE_DIR_TABLE_POS + 0x0], eax		;第0个目录项的内容是0x101007, 记录第0个页表的物理地址0x101000
	; 0xc00 / 4 = 768，即第768个页目录项，指向第0个页表, 操作系统放在了低端1M的物理内存空间，但是操作系统的虚拟地址是0xc0000000以上
	; 这样虚拟地址0xc0000000 ~ 0xc00fffff 对应物理地址0x0 ~ 0xfffff；从而实现了操作系统在高3GB以上的虚拟地址对应到了低端1MB
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax
	
	sub eax, 0x1000							; eax = 0x100007
    ; 页目录表的最后一个页目录项（第1023个）指向页目录表自身的地址;这是为了动态操作页表，这里不太好理解，后续写代码的时候就知道原因了。
    mov [PAGE_DIR_TABLE_POS + 1023 * 4], eax

	
; 创建第0个页表的页表项（PTE），先创建256个页表项，即先将低端1MB的物理地址与虚拟地址建立映射
; 在加载内核之前，程序中运行的一直都是loader，它本身的代码在1MB之内，必须保证之前段机制下的线性地址和分页后的虚拟地址对应的物理地址一致，
; 即虚拟地址0x0 ~ 0xfffff与物理地址0x0 ~ 0xfffff一一对应。
    mov ecx, 256							; 1M / 4K = 256,256个页
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P		; edx = 0x7, 页表的属性	
	
.create_pte:
; ebx = 0x101000，即第0个页表的物理地址，这里是给256个页表项赋值，
; 第0个页表项内容: 0x7，即该页的地址范围是：0x0 ~ 0xfff
; 第1个页表项内容：0x1007，即该页的地址范围是：0x1000 ~ 0x1fff
; ...
; 第255个页表项内容：0xff007，即该页的地址范围是：0xff000 ~ 0xfffff
; 这样虚拟地址0x0 ~ 0xfffff与物理地址0x0 ~ 0xfffff一一对应起来
    mov [ebx + esi*4], edx					
    add edx, 4096
    inc esi
    loop .create_pte

	
; 为了真正实现内核空间被所有用户进程共享，需要在页目录表中把内核空间的页目录项填满。
; 所有用户进程共享内核，各用户进程的高1GB必须都指向内核所在的物理内存空间，也就是说每个进程页目录表中第768~1022个页目录项都是与其他进程相同的
; 为用户进程创建页表时，应该把内核页表中第768 ~ 1022个页目录项复制到用户进程页目录表中的相同位置。
; 一个页目录项对应一个页表地址，页表地址固定了，后来新增的页表项也只会加在这些固定的页表中。
; 实现内核完全共享最简单的办法是提前把内核的所有页目录项定下来，也就是提前把内核的页表固定下来。
; 这样一来，内核所在的空间被所有进程共享，所有进程都可以使用内核提供的服务，
; 内核若为任意一个用户进程在内核空间中创建了某些资源的话，其他进程都可以访问到该资源。
; 创建内核其他页表的PDE, 即从第769个页目录项到第1022个页目录项，共254个页目录项,第768个页目录项已经创建了。
    mov eax, PAGE_DIR_TABLE_POS				; eax = 0x100000
    add eax, 0x2000							; eax = 0x102000
    or eax, PG_US_U | PG_RW_W | PG_P		; eax = 0x102007
    mov ebx, PAGE_DIR_TABLE_POS				; ebx = 0x100000
    mov ecx, 254							; 范围是第769 ~ 1022个页目录项的所有页目录项的数量
    mov esi, 769

.create_kernel_pde;
; 第769个页目录项内容: 0x102007
; 第770个页目录项内容：0x103007
; ...
; 第1022个页目录项内容：0x1ff007
    mov [ebx + esi * 4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde
    ret	
	

;--------------------- 将kernel.bin中的segment拷贝到编译的地址 --------
;对于可执行程序，我们只对其中的段（segment）感兴趣，它们才是程序运行的实质指令和数据的所在地，
;所以我们要找出程序中所有的段
kernel_init:
	xor eax, eax
	xor ebx, ebx			; ebx用来记录程序头表地址
	xor ecx, ecx			; cx用来记录程序头表中的program header数量
	xor edx, edx			; dx用来记录program header尺寸，即e_phentsize
	
	mov dx, [KERNEL_BIN_BASE_ADDR + 42]			; e_phentsize在文件开头42字节处
	mov ebx, [KERNEL_BIN_BASE_ADDR + 28]		; 偏移28字节地方是e_phoff,表示第1个program header在文件中的偏移量
												; 该值为ebx = 0x34
	add ebx, KERNEL_BIN_BASE_ADDR				; ebx = ebx + KERNEL_BIN_BASE_ADDR = 0x70034, 
												; 此时ebx指向程序中第一个program header
	
	mov cx, [KERNEL_BIN_BASE_ADDR + 44]			; 偏移文件44字节的地方是e_phnum,即有几个program header
												; 段是由program header来描述的，即一个程序头代表一个段
												; cx就是段的个数
	
.each_segment:
	cmp byte [ebx + 0], PT_NULL					; 若p_type等于PT_NULL,说明此program header未使用												
	je .PTNULL									; 如果发现该段是空段类型的话，就跨过该段不处理，跳到.PTNULL处
	
	; 为函数memcpy压入参数，参数是从右往左依次压入
	; memcpy(void* dest, void* src, int size)
	push dword [ebx + 16]						; 将该段的大小p_filesz压栈
	mov eax, [ebx + 4];							; 举例程序头偏移量4字节的是p_offset
	add eax, KERNEL_BIN_BASE_ADDR				; eax就是该段的物理地址了,eax = 0 + 0x70000
	push eax									; 压入函数memcpy的第二个参数
	push dword [ebx + 8];						; 压入函数memcpy的第一个参数，目的地址，即p_vaddr
												; p_vaddr = 0xc0001000
	call memcpy									; 调用memcpy完成复制
	add esp, 12									; 清理栈中压入的三个参数
	
	
	
	
	
.PTNULL:
	add ebx, edx								; edx为program header大小，即e_phentsize
												; 在此ebx指向下一个program header
	loop .each_segment
	ret
	
