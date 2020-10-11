#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

void panic_spin(char* filename, int line, const char* func, const char* condition);

/*************************************************************** __VA_ARGS__ ************************************
* __VA_ARGS__ 是预处理器所支持的专用标识符，代表所有与省略号相对应的参数。
* "..."表示定义的宏参数可变。*/
#define PANIC(...) panic_spin(__FILE__, __LINE__, __FUNC__, __VA_ARGS__)
/*****************************************************************************************************************/


// 宏是预处理器提供的功能，#ifdef NDEBUG，这里可以随时使ASSERT失效，即在gcc编译时指定宏NDEBUG，方法很简单，即gcc -D NDEBUG
#ifdef NDEBUG
	#define ASSERT(CONDITION) ((void)0)					// 让assert称为空，什么都不做
#else
#define ASSERT(CONDITION) \
	if(CONDITION) {} else { \ /* 如果CONDITION为真，就什么都不做 */
	PANIC(#CONDITION);		/* 符号#让编译器将宏的参数转化为字符串字面量，如assert(var != 0); #CONDITION的效果是变成了"var != 0" */ \
	}
#endif /* NDEBUG */
#endif /* __KERNEL_DEBUG_H */
