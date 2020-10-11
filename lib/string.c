#include "string.h"
#include "global.h"
#include "debug.h"

/* 内存数据初始化。将dst_起始的size个字节置为value */
void memset(void* dst_, uint8_t value, uint32_t size)
{
    ASSERT(dst_ != NULL);
    uint8_t* dst = (uint8_t*) dst_;
    while(size-- > 0)
        *dst++ = value;
}

/* 内存数据拷贝。将src_起始的size个字节复制到dst_*/
void memcpy(void* dst_, const void* src_, uint32_t size)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    uint8_t* dst = dst_;
    const uint8_t* src = src_;
    while(size-- > 0)
        *dst++ = *src++;
}

/* 内存数据比较。连续比较以地址a_和地址b_开头的size个字节，若相等则返回0，若a_>b_,返回1，否则返回-1*/
int memcmp(const void* a_, const void*  b_, uint32_t size)
{
    const char* a = a_;
    const char* b = b_;
    ASSERT(a != NULL && b != NULL);
    while(size-- > 0){
        // 比较的是ASCII码值
        if(*a != *b) {
        	return *a > *b ? 1 : -1;
        }
        a++;
        b++;
    }
    return 0;
}

/* 将字符串从src_复制到dst_*/
char* strcpy(char* dst_, const char* src_) {
	ASSERT(dst_ != NULL && src_ != NULL);
    char* r = dst_;							// 用来返回字符串的起始地址
    while((*dst_++ = *src_++));				// 以src_处的字符'0'作为终止条件,即当*dst被复制为0时，也就是表达式不成立，正好添加了字符串结尾的0.
    return r;
}

/* 返回字符串长度 */
uint32_t strlen(const char* str)
{
    ASSERT(str != NULL);
    const char* p = str;
    while(*p++);
    return (p - str + 1);
}

// 比较两个字符串，若a_中的字符大于b_中的字符返回1，相等时返回0， 否则返回-1。
int8_t strcmp(const char* a, const char* b)
{
    while(*a != 0 && *a == *b)
    {
        a++;
        b++;
    }
    
    // 如果*a < *b,就返回-1，否则就是*a >= *b, 在后面的布尔表达式"*a > *b"中， 若*a > *b, 表达式等于1，否则，也就是*a == *b, 表达式等于0.
    return *a < *b ? -1 : *a > *b;
}

/* 从左往右查找字符串str中首次出现字符ch的地址 */
char* strchr(const char* str, const uint8_t ch)
{
    while(*str != 0) {
    	if(*str == ch)
            return (char*)str;
        str++;
    }
    return NULL
}


/* 从右往前查找字符串str中首次出现字符ch的地址 */
char* strrchr(const char* str, const uint8_t ch)
{
    const char* last_char = NULL;
    /* 从头到尾遍历一次，若存在ch字符，last_char总是该字符最后一次出现在串中的地址 */
    while(*str != 0)
    {
        if(*str == ch)
        {
            last_char = str;
        }
        str++;
    }
    return (char*)last_char;
}

// 将字符串src_拼接到dst_后，返回拼接的串地址。
char* strcat(char* dst_, const char* src_)
{
    char* str = dst_;
    while(*str++);
    --str;
    while((*str++ = *src_++));			// 当*str被复制为0时，也就是表达式不成立，正好添加了字符串结尾的0.
    return dst_;
}

/* 在字符串str中查找字符ch出现的次数 */
uint32_t strchrs(const char* str, uint8_t ch)
{
    uint32_t ch_cnt = 0;
    const char* p = str;
    while(*p != 0) {
    	if(*p == ch) {
        	ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}
