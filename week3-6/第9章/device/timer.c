#include "timer.h"
#include "io.h"
#include "print.h"

#define IRQ0_FREQUENCY	   100
#define INPUT_FREQUENCY	   1193180
#define COUNTER0_VALUE	   INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT	   0x40
#define COUNTER0_NO	   0
#define COUNTER_MODE	   2
#define READ_WRITE_LATCH   3
#define PIT_CONTROL_PORT   0x43

/*
IRQ0_FREQUENCY 是我们要设置的时钟中断的频率，我们要将它设为 100Hz。
INPUT_FREQUENCY 是计数器 0 的工作脉冲信号频率，前面介绍过。
COUNTER0_VALUE 是计数器 0 的计数初值，这是由之前的公式算出来的，当然了咱们为图省事，直接用 宏来计算啦，所以 COUNTER0_VALUE 的值为 INPUT_FREQUENCY / IRQ0_FREQUENCY。
CONTRER0_PORT 是计数器 0 的端口号 0x40。
COUNTER0_NO 是用在控制字中选择计数器的号码，其值为 0，代表计数器 0，它将被赋值给函数的 形参 counter_no。
COUNTER_MODE 是工作模式的代码，其值为 2，即方式 2。
这是我们选择的工作方式:比率发生器。 READ_WRITE_LATCH 是读写方式，其值为 3，这表示先读写低 8 位，再读写高 8 位。原因很简单，我们要写入的初值是 16 位，按照 8253 的初始化步骤，必须先写低 8 位，后写高 8 位。
PIT_CONTROL_PORT 是控制字寄存器的端口。
*/

static void frequency_set(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value ){
    
    /* 往控制字寄存器端口0x43中写入控制字 */
       outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    /* 先写入counter_value的低8位 */
       outb(counter_port, (uint8_t)counter_value);
    /* 再写入counter_value的高8位 */
       outb(counter_port, (uint8_t)counter_value >> 8);
}

void timer_init() {
   put_str("timer_init start\n");
   /* 设置8253的定时周期,也就是发中断的周期 */
   frequency_set(CONTRER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
   put_str("timer_init done\n");
}
