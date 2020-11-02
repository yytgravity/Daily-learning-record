#include "process.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"    
#include "list.h"    
#include "tss.h"    
#include "interrupt.h"
#include "string.h"
#include "console.h"

extern void intr_exit(void);

//构建用户上下文信息
void start_process(void* filename_){
    void* function = filename_;
    struct task_struct* cur = running_thread();
    cur->self_kstack += sizeof(struct task_struct); //为什么+忘记的话去看笔记。
    struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = proc_stack->ecx =proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_K_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (void* )((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR)+PG_SIZE);
    proc_stack->ss = SELECTOR_U_DATA;
    asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory");
}

//激活页表
void page_dir_activate(struct task_struct* p_thread){
    /********************************************************
    * 执行此函数时,当前任务可能是线程。
    * 之所以对线程也要重新安装页表, 原因是上一次被调度的可能是进程,
    * 否则不恢复页表的话,线程就会使用进程的页表了。
    ********************************************************/
    /*
     目前咱们的线程并不是为用户进程服务的，它是为内核服务的， 因此与内核共享同一地址空间，也就是和内核用的是同一套页表。当进程 A 切换到进程 B 时，页表也要 随之切换到进程 B 所用的页表，这样才保证了地址空间的独立性。当进程 B 又切换到线程 C 时，由于目 前在页表寄存器 CR3 中的还是进程 B 的页表，因此，必须要将页表更换为内核所使用的页表。所以，无 论是针对进程，还是线程，都要考虑页表切换。
     */
    
    //若是内核线程，需要重新填充页表为0x100000
    uint32_t pagedir_phy_addr = 0x100000;// 默认为内核的页目录物理地址,也就是内核线程所用的页目录表
    if(p_thread->pgdir != NULL){
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
    }
    asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

//激活线程或进程的页表，更新tss中的esp0为进程的特权级0的栈。

void process_activate(struct task_struct* p_thread){
    ASSERT(p_thread != NULL);
    
    page_dir_activate(p_thread);
    
    //内核线程特权级本身就是0，处理器在进入中断时并不会从tss中获取0特权级栈地址，所以不需要更新esp0
    if(p_thread->pgdir){
        update_tss_esp(p_thread);
    }
}


//创建页目录表，将当前页表表示的内核空间的pde复制，成功则返回页目录的虚拟地址，否则返回-1
uint32_t* create_page_dir(void){
    //用户的页表不能让用户访问到，所以在内核空间申请
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    if(page_dir_vaddr == NULL){
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }
    /************************** 1  先复制页表  *************************************/
    /*  page_dir_vaddr + 0x300*4 是内核页目录的第768项 */
    memcpy((uint32_t* )((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t *)(0xfffff000+0x300*4), 1024);
    
    /************************** 2  更新页目录地址 **********************************/
    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    
    return page_dir_vaddr;
}

//创建用户进程虚拟地址位图
void create_user_vaddr_bitmap(struct task_struct* user_prog){
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START)/PG_SIZE/8, PG_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

//创建用户进程
void process_execute(void* filename, char* name){
    //pcb内核的数据结构，由内核来维护进程信息，因为要在内核内存池中申请
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, default_prio);
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename);
    thread->pgdir = create_page_dir();
    
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &thread->gernel_tag));
    list_append(&thread_ready_list, &thread->gernel_tag);
    
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
    
}

