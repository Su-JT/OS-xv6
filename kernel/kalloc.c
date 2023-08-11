// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct memref{
  struct spinlock lock;
  int count;
} mem_ref[PHYSTOP/PGSIZE];

void
kinit()
{
  /*lab6  begin*/
  for(int i=0; i < PHYSTOP/PGSIZE; i++)     //初始化
    initlock(&(mem_ref[i].lock), "mem_ref");
  /*lab6  end*/
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  /*lab6  begin*/
  acquire(&(mem_ref[(uint64)pa/PGSIZE].lock));
  mem_ref[(uint64)pa/PGSIZE].count--;
  if(mem_ref[(uint64)pa/PGSIZE].count > 0) 
    goto notzero;
  /*lab6  end*/

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);

notzero:
  release(&(mem_ref[(uint64)pa/PGSIZE].lock));
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){      
    /*lab6  begin*/   //标记分配的页块
    acquire(&(mem_ref[(uint64)r/PGSIZE].lock));
    mem_ref[(uint64)r/PGSIZE].count = 1;
    release(&(mem_ref[(uint64)r/PGSIZE].lock));
    /*lab6  end*/
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


int 
add_ref(uint64 pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  acquire(&(mem_ref[pa/PGSIZE].lock));
  mem_ref[pa/PGSIZE].count ++;
  release(&(mem_ref[pa/PGSIZE].lock));
  return 1;
}
/* 判断是否为copy-on-write */
int
is_cow(pagetable_t pagetable, uint64 addr)
{
   if(addr > MAXVA)
    return 0;

  pte_t* pte = walk(pagetable, addr, 0);
  if(pte == 0 || ((*pte) & PTE_V) == 0)
    return 0;

  return ((*pte) & PTE_COW);
}
uint64
cow_copy(pagetable_t pagetable, uint64 addr)
{
  if(is_cow(pagetable, addr) == 0)
    return 0;

  addr = PGROUNDDOWN(addr);
  pte_t* pte = walk(pagetable, addr, 0);
  uint64 pa = PTE2PA(*pte);
  acquire(&(mem_ref[pa/PGSIZE].lock));
  if(mem_ref[pa/PGSIZE].count == 1){    //仅有一个进程在使用，恢复写入权限
    *pte |= PTE_W;
    *pte &= (~PTE_COW);
    release(&(mem_ref[pa/PGSIZE].lock));
    return pa;
  }
  release(&(mem_ref[pa/PGSIZE].lock));
  char* ka = kalloc();
  if(ka == 0)
    return 0;
  memmove(ka, (char *)pa, PGSIZE);
  *pte &= (~PTE_V);
  uint64 flag = PTE_FLAGS(*pte);
  flag |= PTE_W;
  flag &= (~PTE_COW);
  if(mappages(pagetable, addr, PGSIZE, (uint64)ka, flag)!= 0){
    kfree(ka);
    return 0;
  }
  kfree((char*)PGROUNDDOWN(pa));
  return (uint64)ka;
}