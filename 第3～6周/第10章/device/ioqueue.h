#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

//环形队列
struct ioqueue{
    struct lock lock;
    struct task_struct* producer;
    struct task_struct* consumer;
    char buf[bufsize];
    int32_t head;
    int32_t tail;
};
/*
 lock 是本缓冲区的锁，每次对缓冲区操作时都要先申请这个锁，从而保证缓冲区操作互斥。
 producer 是生产者，此项来记录当缓冲区满时，在此缓冲区睡眠的生产者线程。
 consumer 是消费者，此项来记录当缓冲区空时，在此缓冲区睡眠的消费者线程。
 buf[bufsize]是定义的缓冲区数组，其大小为 bufsize，在上面用 define 定义为 64。
 head 是缓冲区队列的队首地址，tail 是队尾地址。
 */


void ioqueue_init(struct ioqueue* ioq);
bool ioq_full(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);
#endif
