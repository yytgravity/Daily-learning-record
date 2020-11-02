#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "memory.h"

#define PG_SIZE 4096

struct task_struct* main_thread;    // 主线程PCB
struct list thread_ready_list;	    // 就绪队列
struct list thread_all_list;	    // 所有任务队列
static struct list_elem* thread_tag;// 用于保存队列中的线程结点

extern void switch_to(struct task_struct* cur, struct task_struct* next);

//获取当前线程pcb指针
struct task_struct* running_thread(){
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g" (esp)); //取esp整数部分即pcb起始地址
    return (struct task_struct*)(esp & 0xfffff000);
}

/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg) {
/* 执行function前要开中断,避免后面的时钟中断被屏蔽,而无法调度其它线程 */
   intr_enable();
   function(func_arg);
}

/* 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg){
    pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

//初始化线程基本信息
void init_thread(struct task_struct* pthread, char* name, int prio){
    memset(pthread,0,sizeof(*pthread));
    strcpy(pthread->name, name);
    if(pthread == main_thread){
        pthread->status = TASK_RUNNING;
    }else{
        pthread->status = TASK_READY;
    }
    
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->stack_magic = 0x19870916;  //魔术，自己随便定
}

//创建一优先级为prio的线程，线程名为name,线程所执行的函数是function(func_arg)
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg){
    //pcb都位于内核空间，即使用户进程的pcb也是内核空间
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);
    
    ASSERT(!elem_find(&thread_ready_list, &thread->gernel_tag));
    list_append(&thread_all_list, &thread->gernel_tag);
    
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    
    return thread;
}

//将kernel中的main函数完善为主线程
static void make_main_thread(void){
    /* 因为main线程早已运行,咱们在loader.S中进入内核时的mov esp,0xc009f000,
    就是为其预留了tcb,地址为0xc009e000,因此不需要通过get_kernel_page另分配一页*/
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    
    /* main函数是当前线程,当前线程不在thread_ready_list中,
    * 所以只将其加在thread_all_list中. */
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

//任务调度
void schedule(){
    
    ASSERT(intr_get_status() == INTR_OFF);
    
    struct task_struct* cur = running_thread();
    if(cur->status == TASK_RUNNING){
        ASSERT(!elem_find(&thread_ready_list, &cur->gernel_tag));
        list_append(&thread_ready_list, &cur->gernel_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }else{
        
    }
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, gernel_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur, next);
}

void thread_init(void){
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    put_str("thread_init done\n");
}
