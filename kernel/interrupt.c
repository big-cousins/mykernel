#include "interrupt.h"
#include "stdint.h"
#include "global.h"

#define IDT_DESC_CNT 0x21								// 目前总共支持的中断数


// 中断打开时，对应的IF位的值为1
#define EFLAGS_IF 0x00000200			// 中断打开时，eflags寄存器中的if位为1. 由于IF位于eflags的第9位，故值为0x200
// 获取eflags寄存器的值，pushf1将eflags寄存器的值压入栈，popl将其弹出到内存，最后给EFLAGS_VAR变量赋值。
#define GET_EFLAGS(EFLAGS_VAR) asm volatile("pushf1; popl %0" ： "=g" (EFLAGS_VAR))

/* 中断门描述符结构体*/
struct gate_desc {
	uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t  dcount;									// 固定值，不用考虑
    uint8_t  attribute;
    uint16_t func_offset_high_word;
};

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);

static struct gate_desc idt[IDT_DESC_CNT];				//idt是中断描述符表，本质上就是中断门描述符数组

char* intr_name[IDT_DESC_CNT];		     				// 用于保存异常的名字
// 定义中断处理程序数组，在kernel.S中定义的intrXXentry只是中断处理程序的入口，最终调用的是idt_table中的处理程序
intr_handler idt_table[IDT_DESC_CNT];

// intr_handler: typedef void* intr_handler
// intr_entry_table中的元素都是普通地址
extern intr_handler intr_entry_table[IDT_DESC_CNT];		// 声明引用定义在kernel.S中的中断处理函数入口数组


/* 创建中断门描述符 
*  p_gdesc: 中断门描述符指针
*  attr: 中断描述符内的属性
*  function：中断描述符内对应的中断处理函数
*/
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;				// 取地址的低16位
    // SELECTOR_K_CODE是指向内核数据段的选择子
    p_gdesc->selector = SELECTOR_K_CODE;											// SELECTOR_K_CODE = 1000（二进制），这个涉及到GDT之类的，暂时看不懂
    p_gdesc->count = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;		// 取地址的高16位
}

/* 初始化中断描述符表 */
static void idt_desc_init()
{
    int i = 0;
    for(; i < IDT_DESC_CNT; i++)
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
}

/* 完成一般中断处理函数注册及异常名称注册 */
static void exception_init(void) {			    // 完成一般中断处理函数注册及异常名称注册
   int i;
   for (i = 0; i < IDT_DESC_CNT; i++) {

/* idt_table数组中的函数是在进入中断后根据中断向量号调用的,
 * 见kernel/kernel.S的call [idt_table + %1*4] */
      idt_table[i] = general_intr_handler;		    // 默认为general_intr_handler。
							    // 以后会由register_handler来注册具体处理函数。
      intr_name[i] = "unknown";				    // 先统一赋值为unknown 
   }
   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "#BP Breakpoint Exception";
   intr_name[4] = "#OF Overflow Exception";
   intr_name[5] = "#BR BOUND Range Exceeded Exception";
   intr_name[6] = "#UD Invalid Opcode Exception";
   intr_name[7] = "#NM Device Not Available Exception";
   intr_name[8] = "#DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segment Overrun";
   intr_name[10] = "#TS Invalid TSS Exception";
   intr_name[11] = "#NP Segment Not Present";
   intr_name[12] = "#SS Stack Fault Exception";
   intr_name[13] = "#GP General Protection Exception";
   intr_name[14] = "#PF Page-Fault Exception";
   // intr_name[15] 第15项是intel保留项，未使用
   intr_name[16] = "#MF x87 FPU Floating-Point Error";
   intr_name[17] = "#AC Alignment Check Exception";
   intr_name[18] = "#MC Machine-Check Exception";
   intr_name[19] = "#XF SIMD Floating-Point Exception";

}

// 获取当前中断状态
enum intr_status intr_get_status()
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}

// 开中断，并返回开中断前的状态
enum intr_status intr_enable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
    	old_status = INTR_ON;
        return old_status;
    } else {
    	old_status = INTR_OFF;
        asm volatile("sti");			// 开中断，sti指令将IF位置1
        return old_status;
    }
}

// 关中断，并返回关中断前的状态
enum intr_status intr_disable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
    	old_status = INTR_ON;
        asm volatile("cli" : : : "memory");		// 关中断，cli指令将IF位置0
    } else {
    	old_status = INTR_OFF;
    }
    return old_status;
}

// 将中断状态设置为status
enum intr_status intr_set_status(enum intr_status status)
{
    return status & INTR_ON ? intr_enable() : intr_disable();
}

/*完成有关中断的所有初始化工作*/
void idt_init() {
   put_str("idt_init start\n");
   idt_desc_init();	   		// 初始化中断描述符表
   exception_init();	   	// 异常名初始化并注册通常的中断处理函数
   pic_init();		   		// 初始化8259A

   /* 加载idt */
   uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));			// 数组名idt便是地址，即指针
   asm volatile("lidt %0" : : "m" (idt_operand));
   put_str("idt_init done\n");
}
