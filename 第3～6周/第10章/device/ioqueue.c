#include "ioqueue.h"
#include "interrupt.h"
#include "global.h"
#include "debug.h"

// 初始化io队列ioq
void ioqueue_init(struct ioqueue* ioq){
    lock_init(&ioq->lock);
    ioq->producer = ioq->consumer = NULL;
    ioq->head = ioq->tail = 0;
}

//返回pos在缓冲区中的下一个位置值
//将 pos+1 后再对bufsize 求模得到的，这保证了缓冲区指针回绕着数组 buf，从而实现了环形缓冲区。
static int32_t next_pos(int32_t pos){
    return(pos + 1) % bufsize;
}

//判断队列是否已满
bool ioq_full(struct ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    return next_pos(ioq->head) == ioq->tail;
}

//判断队列是否已空
static bool ioq_empty(struct ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    return ioq->head == ioq->tail;
}

//使当前生产者或者消费者在此缓冲区上等待
static void ioq_wait(struct task_struct** waiter){
    ASSERT(*waiter == NULL && waiter !=NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}

//唤醒waiter
static void wakeup(struct task_struct** waiter){
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

//消费者从ioq队列中获取一个字符
char ioq_getchar(struct ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    /*先通过“while(ioq_empty(ioq))”循环判断缓冲区 ioq 是否为空，如果为空就表示没有数据
    可取，只好先在此缓冲区上睡眠，直到有生产者将数据添加到此缓冲区后再被叫醒重新取数据。消费者有可能有多个，它们之间是竞争的关系，醒来后有可能别的消费者刚刚把缓冲区中的数据取走 了，因此在当前消费者被叫醒后还要再判断缓冲区是否为空才比较保险，所以用 while 循环来重复判断。
    while 循环体中先通过“lock_acquire(&ioq->lock)”申请缓冲区的锁，持有锁后，通过“ioq_ wait(&ioq-> consumer)”将自己阻塞，也就是在此缓冲区上休眠。您看，这里传给 ioq_wait 的实参就是缓冲区的消费 者&ioq->consumer，此项用来记录哪个消费者没有拿到数据而休眠。这样等将来某个生产者往缓冲区中添 加数据的时候就知道叫醒它继续拿数据了。醒来后执行“lock_release(&ioq->lock)”释放锁。*/
    while(ioq_empty(ioq)){
        lock_acquire(&ioq->lock);
        ioq_wait(ioq->consumer);
        lock_release(&ioq->lock);
    }
    
    char byte = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);
    
    if(ioq->producer != NULL){
        wakeup(&ioq->producer);
    }
    
    return byte;
}

/*
 当消费者读取一个字节后，缓冲区就腾出一个数据单位的空间了，这时候要判断一下是否有生产者 在此缓冲区上休眠。（若之前此缓冲区是满的，正好有生产者来添加数据，那个生产者一定会在此缓冲区上睡眠。）有的话就把他唤醒。
 
 下面同理，依然要判断是否有消费者在此缓冲区上休眠。若之前此缓冲区为空，恰好有消费者来取数据，因此 会导致消费者休眠。现在当前生产者线程已经往缓冲区中添加了数据，现在可以将消费者唤醒让它继续取数 据了。
 */

//生产者往ioq队列写入一个字符byte
void ioq_putchar(struct ioqueue* ioq, char byte){
    ASSERT(intr_get_status() == INTR_OFF);
    while(ioq_full(ioq)){
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }
    
    ioq->buf[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);
    
    if(ioq->consumer == NULL){
        wakeup(&ioq->consumer);
    }
}
