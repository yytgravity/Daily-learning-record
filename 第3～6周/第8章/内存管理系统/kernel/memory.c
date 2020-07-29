#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"

#define PG_SIZE 4096

/***************  位图地址 ********************
 * 因为0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb.
 * 一个页框大小的位图可表示128M内存, 位图位置安排在地址0xc009a000,
 * 这样本系统最大支持4个页框的位图,即512M */
#define MEM_BITMAP_BASE 0xc009a000
/*************************************/

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

/* 0xc0000000是内核从虚拟地址3G起. 0x100000意指跨过低端1M内存,使虚拟地址在逻辑上连续 */
#define K_HEAP_START 0xc0100000

/*内存池结构，生成两个实例用于管理内核内存池和用户内存池*/
struct pool{
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
}

struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

/* 在pf表示的虚拟内存池中申请pg_cnt个虚拟页,成功则返回虚拟页的起始地址, 失败则返回NULL */
static void* vadder_get(enum pool_flags pf, uint32_t pg_cnt){
    int vaddr_start = 0,bit_idx_start = -1;
    uint32_t cnt = 0;
    if(pf == PF_KERNEL){
        bit_idx_start = bitmap_scan(&kernel_vadder.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1){
            return NULL:
        }
        while(cnt < pg_cnt){
            bitmap_set(&kernel_vadder.vadder_bitmap,bit_idx_start + cnt++ ,1);
        }
        vadder_start = kernel_vadder.vadder_start + bit_idx_start * PG_SIZE;
    }else{
       // 用户内存池,将来实现用户进程再补充
    }
    return (void*)vadder_start;
}

uint32_t * pte_ptr(uint32_t vaddr){
    uint32_t* pte = (uint32_t)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

uint32_t * pde_pte(uint32_t vaddr){
    uint32_t* pde = (uint32_t)((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}

/*在m_pool指向的物理内存池中分配1个物理页,成功则返回页框的物理地址,失败则返回NULL*/
static void* palloc(struct pool* m_pool){
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1){
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx , 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void*)page_phyaddr;
}

/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr){
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t pde = pde_ptr(vaddr);
    uint32_t pte = pte_ptr(vaddr);
    
    if(*pde & 0x00000001){     //页表项和页目录项的第0位为p，此处判断页目录项是否存在
        ASSERT(!(*pte & 0x00000001));
        
        if(!(*pte & 0x00000001)){
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }else{
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
        }
    }else{
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        memset((void*)((int)pte & 0xfffff000), 0 , PG_SIZE);
        
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
      vaddr += PG_SIZE;         // 下一个虚拟页
   }
   return vaddr_start;
}

/* 从内核物理内存池中申请pg_cnt页内存,成功则返回其虚拟地址,失败则返回NULL */

void* get_kernel_pages(uint32_t pg_cnt){
    void* vaddr = malloc_page(PF_KERNEL,pg_cnt);
    if(vaddr != NULL){
        memset(vaddr, 0 , pg_cnt * PG_SIZE);
    }
    return vaddr;
}

/* 初始化 */

static void mem_pool_init(uint32_t all_mem){
    put_str("   mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;
    // 页表大小= 1页的页目录表+第0和第768个页目录项指向同一个页表+第769~1022个页目录项共指向254个页表,共256个页框
    uint32_t used_mem = page_table_size + 0x100000; //0x100000 为低端1M内存
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;
    
    uint16_t kernel_free_pages = all_free_pages /2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;
    
    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;
    
    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE ;
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length);
    
    put_str("      kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("      user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_str("\n");
    
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);
    
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    
    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}

void mem_iinit(){
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
    //0xb00地址处存储的是之前获取的内存容量。
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done\n");
}
