## 锁

#### 临界区、互斥和竞争条件

- 公共资源
  - 可以是公共内存、公共文件、公共硬件等，总之是被所有任务共享的一套资源。
- 临界区 
  - 程序要想使用某些资源，必然通过一些指令去访问这些资源，若多个任务都访问同一公共资源，那么各任务中访问公共资源的指令代码组成的区域就称为临界区。
  - 注意！注意！⚠️临界区是指程序中那些访问公共资源的指令代码，即临界区是指令，并不是受访的静态公共资源。
- 互斥
   - 互斥也可称为排他，是指某一时刻公共资源只能被一个任务独享，即不允许多个任务同时出现在自己的临界区中。公共资源在任意时刻只能被一个任务访问，即只能有一个任务在自己的临界区中执行，其他任务想访问公 共资源时，必须等待当前公共资源的访问者完全执行完他自己的临界区代码后(使用完资源后)再开始访问。
- 竞争条件
   - 竞争条件是指多个任务以非互斥的方式同时进入临界区，大家对公共资源的访问是以竞争的方式并行 进行的，因此公共资源的最终状态依赖于这些任务的临界区中的微操作执行次序。
   - 当多个任务“同时”读写公共资源时，也就是多个任务“同时”执行它们各自临界区中的代码时，它们以 混杂并行的方式访问同一资源，因此后面任务会将前一任务的结果覆盖，最终公共资源的结果取决于所有任务 的执行时序。这里所说的“同时”也可以指多任务伪并行，总之是指一个任务在自己的临界区中读写公共资 源，还没来得及出来(彻底执行完临界区所有代码)，另一个任务也进入了它自己的临界区去访问同一资源。

- 热知识
   - 关中断是实现互斥最简单的方法，我们之后实现的各种互斥手段也将以它为基础。

#### 信号量
- 简介
   - 信号量就是个计数器，它的计数值是自然数，用来记录所积累信号的数量。这里的 信号是个泛指，取决于信号量的实际应用环境。可以认为是商品的剩余量、假期剩余的天数、账号上的余 额等，总之，信号的意义取决于您用信号量来做什么，信号量仅仅是一种程序设计构造方法。

- 信号量p（减少）、v（增加）操作
   - 增加操作up包括两个微操作：
      - 将信号量的值加一。
      - 唤醒在此信号量上等待的线程。
   - 减少操作down包括三个字操作
      - 判断信号量是否大于0。
      - 若信号量大于0，则信号量减1。
      - 若信号量等于0，当前线程将自己阻塞，以在此信号量上等待。      

- 信号量的初值代表是信号资源的累积量，也就是剩余量，若初值为 1 的话，它的取值就只能为 0 和 1， 这便称为二元信号量，我们可以利用二元信号量来实现锁。在二元信号量中，down 操作就是获得锁，up 操作就是释放锁。我们可以让线程通过锁进入临界区， 可以借此保证只有一个线程可以进入临界区，从而做到互斥。

#### 线程的阻塞与唤醒

- 阻塞
   - 调度器的功能只是去挑选哪个线程运行，即使再差的调度算法也会保证每个线程都有运行的机会，哪 怕只是运行几个时钟周期。因此，调度器并不决定线程是否可以运行，只是决定了运行的时机，线程可否 运行是由线程自己把控的。当线程被换上处理器运行后，在其时间片内，线程将主宰自己的命运。阻塞是 一种意愿，表达的是线程运行中发生了一些事情，这些事情通常是由于缺乏了某些运行条件造成的，以至 于线程不得不暂时停下来，必须等到运行的条件再次具备时才能上处理器继续运行。因此，阻塞发生的时 间是在线程自己的运行过程中，是线程自己阻塞自己，并不是被谁阻塞。

- 唤醒
  - 已被阻塞的线程是无法运行的，属于睡梦中，因此它只能等待锁的持有者，它释放了锁之后便去唤醒在它后面因获取该锁而阻塞的线程。因此唤醒已阻塞的线程是由别的线程，通常是锁的持有者来做的。  

  - 注意：线程阻塞是线程执行时的“动作”，因此线程的时间片还没用完，在唤醒之后，线程会 继续在剩余的时间片内运行，调度器并不会将该线程的时间片“充满”，也就是不会再用线程的优先级 priority 为时间片 ticks 赋值。因为阻塞是线程主动的意愿，它也是“迫于无奈”才“慷慨”地让出处理器 资源给其他线程，所以调度器没必要为其“大方”而“赏赐”它完整的时间片。

- 代码

```
/* 当前线程将自己阻塞,标志其状态为stat. */
void thread_block(enum task_status stat) {
/* stat取值为TASK_BLOCKED,TASK_WAITING,TASK_HANGING,也就是只有这三种状态才不会被调度*/
   ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));
   enum intr_status old_status = intr_disable();
   struct task_struct* cur_thread = running_thread();
   cur_thread->status = stat; // 置其状态为stat 
   schedule();		      // 将当前线程换下处理器
/* 待当前线程被解除阻塞后才继续运行下面的intr_set_status */
   intr_set_status(old_status);
}

/* 将线程pthread解除阻塞 */
void thread_unblock(struct task_struct* pthread) {
   enum intr_status old_status = intr_disable();
   ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
   if (pthread->status != TASK_READY) {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if (elem_find(&thread_ready_list, &pthread->general_tag)) {
	       PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);    // 放到队列的最前面,使其尽快得到调度
        pthread->status = TASK_READY;
   } 
   intr_set_status(old_status);
}
```

#### 锁的实现

- 信号量down操作

```
/* 信号量down操作 */
void sema_down(struct semaphore* psema) {
/* 关中断来保证原子操作 */
   enum intr_status old_status = intr_disable();
   while(psema->value == 0) {	// 若value为0,表示已经被别人持有
      ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
      /* 当前线程不应该已在信号量的waiters队列中 */
      if (elem_find(&psema->waiters, &running_thread()->general_tag)) {
	 PANIC("sema_down: thread blocked has been in waiters_list\n");
      }
/* 若信号量的值等于0,则当前线程把自己加入该锁的等待队列,然后阻塞自己 */
      list_append(&psema->waiters, &running_thread()->general_tag); 
      thread_block(TASK_BLOCKED);    // 阻塞线程,直到被唤醒
   }
/* 若value为1或被唤醒后,会执行下面的代码,也就是获得了锁。*/
   psema->value--;
   ASSERT(psema->value == 0);	    
/* 恢复之前的中断状态 */
   intr_set_status(old_status);
}
```
- 问：为什么在上面判断信号量是否为0时，用的是while而不是if？
   - 锁本身也是公共的资源，大家也要通过竟争的方式去获得它，因此想要获得锁的线程不只一个，当阻塞 的线程被唤醒后，也不一定就能获得资源，只是再次获得了去竞争锁的机会而已，所以判断信号量的值最好 用 while，而不是用 if。直观上理解，就是判断的次数不同，线程用 while，可以在被唤醒后再次做条件判断， 而 if 则只能判断一次，这就是最大的区别。举个例子：
     - 比如现在有 3 个线程，分别是 t_a，t_b，t_c。假如目前锁由线程 t_a 持有，因此锁中信号量的值为 0。 t_b 也来申请锁，但由于信号量的值为 0，故 t_b 阻塞。当线程 t_a 执行完临界区代码后它会释放锁，释放 锁的操作包括两件事。
          - (1)使信号量的值恢复为 1。 
          - (2)唤醒阻塞的线程。
     
     - 这里将会唤醒线程 t_b。此后，t_b 将会在将来某一时间恢复运行，继续抢锁。正巧的是线程 t_c 也来申请锁了，它比 t_b 先得到调度，因此，它抢先获得了锁。此时信号量又变成了 0。时光飞逝，终于线程 t_b 又被调度上处理器了。
     - 如果之前是用 if(psema->value = = 0)来判断信号量的 value 是否为 0，线程 t_b 醒来的第一件事就是:
         - 执行 psema->value--，使 value 减 1。但它不知 value 之前已经被 t_c 置为 0 了，并不是 1，您看，这就 错了吧。虽然 value 是 uint8_t 类型，值不会为负，但值会变成 8 位宽度的最大值 255，这更不对了。

     - 如果之前是用while(psema->value==0)判断信号量的value是否为0，那线程t_b醒来的第一件事就是:
         - 再次执行while循环的判断psema->value == 0，确认下value是否变成1了，如果依然为0，继续执行 阻塞相关的工作。如果不为 0，则执行 psema->value--，使 value 减 1 为 0，表示获得了锁。
    - 锁的竞争者太多了，并不是说线程在唤醒后，锁就在那闲着等着它来拿。必须确保锁是闲着的才行，因此线程醒来后依然对信号量做判断，我们必须用while。

    
- 信号量的up操作
    
```
* 信号量的up操作 */
void sema_up(struct semaphore* psema) {
/* 关中断,保证原子操作 */
   enum intr_status old_status = intr_disable();
   ASSERT(psema->value == 0);	    
   if (!list_empty(&psema->waiters)) {
      struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
      thread_unblock(thread_blocked);
   }
   psema->value++;
   ASSERT(psema->value == 1);	    
/* 恢复之前的中断状态 */
   intr_set_status(old_status);
}

/* 获取锁plock */
void lock_acquire(struct lock* plock) {
/* 排除曾经自己已经持有锁但还未将其释放的情况。*/
   if (plock->holder != running_thread()) { 
      sema_down(&plock->semaphore);    // 对信号量P操作,原子操作
      plock->holder = running_thread();
      ASSERT(plock->holder_repeat_nr == 0);
      plock->holder_repeat_nr = 1;
   } else {
      plock->holder_repeat_nr++;
   }
}

/* 释放锁plock */
void lock_release(struct lock* plock) {
   ASSERT(plock->holder == running_thread());
   if (plock->holder_repeat_nr > 1) {
      plock->holder_repeat_nr--;
      return;
   }
   ASSERT(plock->holder_repeat_nr == 1);

   plock->holder = NULL;	   // 把锁的持有者置空放在V操作之前
   plock->holder_repeat_nr = 0;
   sema_up(&plock->semaphore);	   // 信号量的V操作,也是原子操作
}

```

- 函数 lock_acquire 接受一个参数，plock 是所要获得的锁，函数功能是获取锁 plock。有时候，线程可能 会嵌套申请同一把锁，这种情况下再申请锁，就会形成死锁，即自己在等待自己释放锁。因此，在函数开头 先判断自己是否已经是该锁的持有者，即代码 if (plock->holder != running_thread())。如果持有者已经是自己， 就将变量 holder_repeat_nr++，除此之外什么都不做，然后函数返回。如果自己尚未持有此锁的话，通过 sema_down(&plock->semaphore)将锁的信号量减 1，当然在 sema_down 中有可能会阻塞，不过早晚会成功返回 的。成功后将当前线程记为锁的持有者，即 plock->holder = running_thread()，然后将 holder_repeat_nr 置为 1， 表示第 1 次申请了该锁。

- 函数 lock_release 只接受一个参数，plock 指向待释放的锁，函数功能是释放锁 plock。当前线程应该 是锁的持有者，所以用 ASSERT 判断了一下。如果持有者的变量 holder_repeat_nr 大于 1，这说明自已多 次申请该锁，此时还不能真正将锁释放，因此只是将 holder_repeat_nr--，随后返回。如果锁持有者的变量 holder_repeat_nr 为 1，说明现在可以释放锁了，通过代码 plock->holder = NULL 将持有者置空，随后将 holder_repeat_nr 置为 0，最后通过“sema_up(&plock->semaphore)”将信号量加 1，自此，锁被真正释放。

- 注意，要把持有者置空语句“plock->holder = NULL”放在 sema_up 操作之前。原因是释放锁的操作 并不在关中断下进行，有可能会被调度器换下处理器。若 sema_up 操作在前的话，sema_up 会先把 value 置 1，若老线程刚执行完 sema_up，还未执行“plock->holder = NULL”便被换下处理器，新调度上来的进程 有可能也申请了这个锁，value 为 1，因此申请成功，锁的持有者 plock->holder 将变成这个新进程的 PCB。 假如这个新线程还未释放锁又被换下了处理器，老线程又被调度上来执行，它会继续执行“plock->holder = NULL”，将持有者置空，这就乱了。

