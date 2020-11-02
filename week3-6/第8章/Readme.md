## 内存管理系统

##### 手稿
![](./img/1.png)

##### 代码+注释部分（只记了重点函数）

###### 一、
```
/* 在位图中申请连续cnt个位,返回其起始位下标 */
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
   uint32_t idx_byte = 0;	 // 用于记录空闲位所在的字节
/* 先逐字节比较,蛮力法 */
   while (( 0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len)) {
/* 1表示该位已分配,所以若为0xff,则表示该字节内已无空闲位,向下一字节继续找 */
      idx_byte++;
   }

   ASSERT(idx_byte < btmp->btmp_bytes_len);
   if (idx_byte == btmp->btmp_bytes_len) {  // 若该内存池找不到可用空间		
      return -1;
   }

 /* 若在位图数组范围内的某字节内找到了空闲位，
  * 在该字节内逐位比对,返回空闲位的索引。*/
   int idx_bit = 0;
 /* 和btmp->bits[idx_byte]这个字节逐位对比 */
   while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]) { 
	 idx_bit++;
   }
	 
   int bit_idx_start = idx_byte * 8 + idx_bit;    // 空闲位在位图内的下标
   if (cnt == 1) {
      return bit_idx_start;
   }

   uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);   // 记录还有多少位可以判断
   uint32_t next_bit = bit_idx_start + 1;
   uint32_t count = 1;	      // 用于记录找到的空闲位的个数

   bit_idx_start = -1;	      // 先将其置为-1,若找不到连续的位就直接返回
   while (bit_left-- > 0) {
      if (!(bitmap_scan_test(btmp, next_bit))) {	 // 若next_bit为0
	 count++;
      } else {
	 count = 0;
      }
      if (count == cnt) {	    // 若找到连续的cnt个空位
	 bit_idx_start = next_bit - cnt + 1;
	 break;
      }
      next_bit++;          
   }
   return bit_idx_start;
}

```
- bitmap_scan_test函数接受两个参数，分别是位图指针btmp和位个数cnt。此函数的功能是在位图btmp中找到连续的cnt个可用位，返回起始空闲位下标。
- 实现的原理是：在btmp->bits所指向的位图中先逐字节查找值为0的位所在的字节，以该字节为起始，然后逐个判断每一个位，若找到连续的cnt值为0的空闲位，返回第一个空闲位所在的下标。

###### 二、
```
/* 得到虚拟地址vaddr对应的pte指针*/
uint32_t* pte_ptr(uint32_t vaddr) {
   /* 先访问到页表自己 + \
    * 再用页目录项pde(页目录内页表的索引)做为pte的索引访问到页表 + \
    * 再用pte的索引做为页内偏移*/
   uint32_t* pte = (uint32_t*)(0xffc00000 + \
	 ((vaddr & 0xffc00000) >> 10) + \
	 PTE_IDX(vaddr) * 4);
   return pte;
}

/* 得到虚拟地址vaddr对应的pde的指针 */
uint32_t* pde_ptr(uint32_t vaddr) {
   /* 0xfffff是用来访问到页表本身所在的地址 */
   uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
   return pde;
}
```
- 首先先来复习一下如何从虚拟地址对应到物理地址：
   - 首先处理高10位的pde索引，从而获取页表的物理地址。
   - 其次是处理中间10位的pte索引，进而处理器得到普通物理页的物理地址。
   - 最后是把低12位作为普通页的页内偏移地址，此偏移地址加上物理页的物理地址，就是最终的物理地址。

- 得到虚拟地址对应的pte
   - 由于最后一个页目录项保存的正是页目录表物理地址，我们让地址的高10位指向最后一个页目录项，即第1023个pde，这样才能获取页目录项表本身的物理地址。1023换成16进制是0x3ff，将其移到高10后，变成0xffc00000。
   - 中间10位是页表项的索引，用来在页表中定位页表项pte。我们在上一步已经使处理器把上一步获得的页目录表当成了页表，，我们之后把vaddr的pde索引当作处理器视角的pte索引就可以了，所以我们将参数vaddr的高十位（pde索引）取出来，做新地址new_vaddr的中间10位（pte索引）。
   - 我们只要把vaddr的中间十位转换成处理器眼中的12位长度的页内偏移量，就可以获得pte了。但是有一点需要注意，地址的低12位寻址范围正好是一页的4kb大小，故处理器直接拿低12位去寻址，不会再为其乘4，所以这里要手动将vaddr的pte部分乘4，即PTE_IDX(vaddr)*4拼凑出了新的虚拟地址new_vaddr的低12位。

- 得到虚拟地址对应的pde
   - 首先第一步同上
   - 第二步，由于要访问的是vaddr所在的页目录项pde，所以要让处理器pte索引时获得的是目录表物理地址，经过上面的铺垫，我们应该很容易就想到了将地址的高20位设置为0xffff，这样就达到了访问到的是最后一个页目录项，即获得了页目录表物理地址。（0xfffffxxx的高十位是0x3ff，中间十位也是0x3ff）
   - 最后一步只要最后再加上vaddr的页目录索引乘以4的积（PDE_IDX(vaddr)*4）。这样就得到了虚拟地址所对应的pde。

   
###### 三、
```
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
   int vaddr_start = 0, bit_idx_start = -1;
   uint32_t cnt = 0;
   if (pf == PF_KERNEL) {
      bit_idx_start  = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
      if (bit_idx_start == -1) {
	 return NULL;
      }
      while(cnt < pg_cnt) {
	 bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
      }
      vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
   } else {	
   // 用户内存池,将来实现用户进程再补充
   }
   return (void*)vaddr_start;
}



/* 在m_pool指向的物理内存池中分配1个物理页,
 * 成功则返回页框的物理地址,失败则返回NULL */
static void* palloc(struct pool* m_pool) {
   /* 扫描或设置位图要保证原子操作 */
   int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);    // 找一个物理页面
   if (bit_idx == -1 ) {
      return NULL;
   }
   bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);	// 将此位bit_idx置1
   uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
   return (void*)page_phyaddr;
}


/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
   uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
   uint32_t* pde = pde_ptr(vaddr);
   uint32_t* pte = pte_ptr(vaddr);

/************************   注意   *************************
 * 执行*pte,会访问到空的pde。所以确保pde创建完成后才能执行*pte,
 * 否则会引发page_fault。因此在*pde为0时,*pte只能出现在下面else语句块中的*pde后面。
 * *********************************************************/
   /* 先在页目录内判断目录项的P位，若为1,则表示该表已存在 */
   if (*pde & 0x00000001) {	 // 页目录项和页表项的第0位为P,此处判断目录项是否存在
      ASSERT(!(*pte & 0x00000001));

      if (!(*pte & 0x00000001)) {   // 只要是创建页表,pte就应该不存在,多判断一下放心
	 *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);    // US=1,RW=1,P=1
      } else {			    //应该不会执行到这，因为上面的ASSERT会先执行。
	 PANIC("pte repeat");
	 *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
      }
   } else {			    // 页目录项不存在,所以要先创建页目录再创建页表项.
      /* 页表中用到的页框一律从内核空间分配 */
      uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);

      *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

      /* 分配到的物理页地址pde_phyaddr对应的物理内存清0,
       * 避免里面的陈旧数据变成了页表项,从而让页表混乱.
       * 访问到pde对应的物理地址,用pte取高20位便可.
       * 因为pte是基于该pde对应的物理地址内再寻址,
       * 把低12位置0便是该pde对应的物理页的起始*/
      memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
         
      ASSERT(!(*pte & 0x00000001));
      *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
   }
}



/* 分配pg_cnt个页空间,成功则返回起始虚拟地址,失败时返回NULL */
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
   ASSERT(pg_cnt > 0 && pg_cnt < 3840);
/***********   malloc_page的原理是三个动作的合成:   ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
***************************************************************/
   void* vaddr_start = vaddr_get(pf, pg_cnt);
   if (vaddr_start == NULL) {
      return NULL;
   }

   uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
   struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

   /* 因为虚拟地址是连续的,但物理地址可以是不连续的,所以逐个做映射*/
   while (cnt-- > 0) {
      void* page_phyaddr = palloc(mem_pool);
      if (page_phyaddr == NULL) {  // 失败时要将曾经已申请的虚拟地址和物理页全部回滚，在将来完成内存回收时再补充
	 return NULL;
      }
      page_table_add((void*)vaddr, page_phyaddr); // 在页表中做映射 
      vaddr += PG_SIZE;		 // 下一个虚拟页
   }
   return vaddr_start;
}
``` 

- malloc_page相当于干了三件事：
   - 通过vaddr_get在虚拟内存池中申请虚拟地址
   - 通过palloc在物理内存池中申请物理页
   - 通过page_table_add将以上两步得到的虚拟地址和物理地址在页表中完成映射。

  