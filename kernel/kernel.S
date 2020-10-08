[bits 32]
%define ERROR_CODE nop		 		; 若在相关的异常中cpu已经自动压入了错误码,为保持栈中格式统一,这里不做操作.
%define ZERO push 0		 			; 若在相关的异常中cpu没有压入错误码,为了统一栈中格式,就手工压入一个0

extern idt_table		 			;idt_table是C中注册的中断处理程序数组

extern put_str			 			;声明外部函数

section .data
	intr_str db "interrupt occur!", 0xa, 0
    global intr_entry_table			; 用global到处位全局符号，作为全局数组，其他的程序就可以使用该数组了。
    intr_entry_table:				; intr_entry_table数组中存的是intr%1entry标号，即中断处理程序的地址

%macro VECTOR 2
section .text						; 汇编中的section（节），用来定义一段相同属性的数据，该范围起始于当前section的定义处，一直持续到下一个section的定义处，若没有遇到新的section定义，则一直持续到文件结束处。
	intr%1entry:					; intr%1entry是标号，也就是地址，这是中断处理程序的起始处，此标号是为了获取中断处理程序的地址
		%2							; nop或push 0
		
		; 以下是保存上下文环境, 保存当前所使用的寄存器环境
        push ds
        push es
        push fs
        push gs
        pushad
        
        ; 如果是从片上进入的中断
        mov al, 0x20
        out 0xa0, al
        out 0x20, al
		
		push %1						; 不管idt_table中的目标程序是否需要参数,都一律压入中断向量号,调试时很方便		
		call [idt_table + %1*4]		; 调用idt_table中的C版本中断处理函数
        jmp intr_exit
section .data						; 
	dd intr%1entry					; 存储各个中断入口程序的地址；形成intr_entry_table数组
    								; 编译器会将属性相同的section合并到同一个大的section中，编译后，所有中断处理程序的地址都会作为数组intr_entry_table的元素紧凑的排在一起。
%endmacro

section .text
	global intr_exit
    intr_exit:
		; 以下是恢复上下文环境
        add esp, 4					; 跳过中断号
        popad
        pop gs
        pop fs
        pop es
        pop ds
        add esp, 4					; 跳过error_code
        iretd						; 从中断返回，即pop EIP, pop cs, pop eflags

VECTOR 0x00, ZERO
VECTOR 0x01, ZERO
VECTOR 0x02, ZERO
...
VECTOR 0x1e, ERROR_CODE
VECTOR 0x1f, ZERO
VECTOR 0x20, ZERO
