#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"


/* 将位图初始化 */
void bitmap_init(struct bitmap* btmp) {
	memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

/* 判断bit_idx位是否为1，若为1，则返回true，否则返回false */
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8;				// 向下取整用于索引数组下标
    uint32_t bit_odd = bit_idx % 8;					// 取余用于索引数组内的位
    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

/* 在位图中申请连续cnt个位，成功，则返回其起始位下标，失败返回-1 */
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
    /*****************************************************************
    * 1，物理内存池中，不会要求连续的位，所以先考虑找到第一个空闲的位，一字节字节的找
    * 2，虚拟内存池中，根据第一个空闲位来继续找，如果连续中断了，则重新开始找，一位位的找
    ******************************************************************/
	
    // 1, 如果是找一位
    uint32_t idx_byte = 0;				// 用于记录空闲位所在的字节
    // 先逐字节比较，蛮力法
    while((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len)) {
        // 若为0xff，则表示该字节内已无空闲位，向下一字节继续找
    	idx_byte++;
    }
    // 没找到
    if(idx_byte == btmp->btmp_bytes_len)
        return -1;
    
    // 在字节内逐位比较，返回空闲位的索引
    int idx_bit = 0;
    while((uint8_t)(BITMAP_MASK << idx_bit) && btmp->bits[idx_byte])
        idx_bit++;
    
    int bit_idx_start = idx_byte * 8 + idx_bit;				// 空闲位在位图中的下标
    if(cnt == 1)
        return bit_idx_start;
    
    // 2， 如果找多个连续的位失败，就重新找，一位位的找
    // 所有剩余的位
    uint32_t bit_left = btmp->btmp_bytes_len * 8 - bit_idx_start;
    
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1;						// 记录已找到的空闲位的个数
    
    bit_idx_start = -1;						// 先将其置为-1， 若找不到连续的位就直接返回
    while(bit_left -- > 0) {
    	if(!(bitmap_scan_test(btmp, next_bit))) {
        	// 若next_bit位为0
            count++;
        } else 
            count = 0;						// 重置
        
        if(count == cnt) {
        	// 若找到连续的cnt个空位
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }
    
    return bit_idx_start;
    
    
    
}

/* 将位图btmp的bit_idx位设置为value */
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value)
{
    uint32_t byte_idx = bit_idx / 8;			// 向下取整用于索引数组下标
    uint32_t bit_odd = bit_idx % 8;				// 取余用于索引数组内的位
    
    if(value)									// 若value为1
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    else										// 若value为0
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
}
