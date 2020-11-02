/**************	 机器模式   ***************
	 b -- 输出寄存器QImode名称,即寄存器中的最低8位:[a-d]l。
	 w -- 输出寄存器HImode名称,即寄存器中2个字节的部分,如[a-d]x。

	 HImode
	     “Half-Integer”模式，表示一个两字节的整数。 
	 QImode
	     “Quarter-Integer”模式，表示一个一字节的整数。 
*******************************************/ 
#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"

static inline void outb(uint16_t port,uint8_t data){
    asm volatile (“outb %b0, %w1” : : "a"(data), "Nd" (port));
}

static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt){
    asm volatile ("cld; rep outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}

static inline uint8_t inb(uint16_t port){
    uint8_t data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void insw(uint16_t port, void* addr, uint32_t word_cnt){
    asm volatile ("cld; rep insw" : "+D" (addr), "+c" (word_cnt) : "d" (port) : "memory");
}

#endif
