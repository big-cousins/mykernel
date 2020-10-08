
;------------------------ loader 和 kernel -----------------------
LOADER_BASE_ADDR 	equ 0x900					; LOADER_BASE_ADDR是loader的起始地址，即0x900
LOADER_STACK_TOP equ LOADER_BASE_ADDR
LOADER_START_SECTOR equ 0x2						; loader放在第2个扇区

KERNEL_START_SECTOR equ 0x9
KERNEL_BIN_BASE_ADDR equ 0x70000

PAGE_DIR_TABLE_POS equ 0x100000					; 页目录表的物理地址，即低端1MB空间的第一个字节
KERNEL_ENTRY_POINT equ 0xc0001500

;-------------   页表配置   ----------------
PAGE_DIR_TABLE_POS equ 0x100000


;--------------   gdt描述符属性  -----------
DESC_G_4K   		equ	  	1_0000_0000_0000_0000_0000_000b	; 高4字节23位为1，表示段界限粒度为4K
DESC_D_32   		equ	   	1_0000_0000_0000_0000_0000_00b	; 高4字节22位为1，表示有效地址和操作数为32位
DESC_L	    		equ	    0_0000_0000_0000_0000_0000_0b	; 高4字节21位为0，表示32位代码。
DESC_AVL    		equ	    0_0000_0000_0000_0000_0000b		; 高4字节20位为0，cpu不用此位，暂置为0

DESC_LIMIT_CODE2  	equ 	1111_0000_0000_0000_0000b		; 高4字节19~16位，代码段界限
DESC_LIMIT_DATA2  	equ 	DESC_LIMIT_CODE2				; 数据段界限
DESC_LIMIT_VIDEO2  	equ 	0000_0000_0000_0000_0000b		; 显存段界限

DESC_P	    		equ		1_0000_0000_0000_000b			; 高4字节15位，为1表示存在内存中


DESC_DPL_0  		equ		00_0000_0000_0000_0b			; 高4字节14~13位，特权级
DESC_DPL_1  		equ		01_0000_0000_0000_0b			;
DESC_DPL_2  		equ		10_0000_0000_0000_0b			;
DESC_DPL_3  		equ		11_0000_0000_0000_0b			;

DESC_S_CODE 		equ		1_0000_0000_0000b				; 高4字节12位，非系统级
DESC_S_DATA 		equ	  	DESC_S_CODE
DESC_S_sys  		equ		0_0000_0000_0000b				; 系统级

; 高4字节11~8位
DESC_TYPE_CODE  	equ	    1000_00000000b					;x=1,c=0,r=0,a=0 代码段是可执行的,非依从的,不可读的,已访问位a清0.  
DESC_TYPE_DATA  	equ	    0010_00000000b					;x=0,e=0,w=1,a=0 数据段是不可执行的,向上扩展的,可写的,已访问位a清0.

; 代码段高4字节，DPL为0
DESC_CODE_HIGH4 	equ 	(0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0x00
; 数据段高4字节，DPL为0
DESC_DATA_HIGH4 	equ 	(0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00
; 显存段高4字节，DPL为0
DESC_VIDEO_HIGH4 	equ 	(0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00



;--------------   选择子属性  ---------------
RPL0  				equ   	00b
RPL1  				equ   	01b
RPL2  				equ   	10b
RPL3  				equ   	11b
TI_GDT	 			equ   	000b
TI_LDT	 			equ   	100b


;----------------   页表相关属性    --------------
PG_P  equ   1b
PG_RW_R	 equ  00b							; 可读 
PG_RW_W	 equ  10b 							; 可写
PG_US_S	 equ  000b							; 该PTE或PDE对应的内存不能被特权级为3的任务访问 
PG_US_U	 equ  100b 							; 


;-------------  program type 定义   --------------
PT_NULL equ 0

