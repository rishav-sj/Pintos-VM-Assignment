#include "vm/frame.h"
#include <debug.h>
#include "threads/malloc.h"
#include "userprog/debug_helper.h"
#include "threads/vaddr.h"
/* #include "userprog/process.c" */
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/synch.h"
#include "vm/pagetable.h"

struct lock framelock;

struct framelist{
  void *upage;
  void *kpage;
  struct framelist* next;
};
struct framelist* F_Table;

void frame_init(){
  lock_init(&framelock);
  SPT_init();
}

void add_list(void *kpage,void *upage){
  lock_acquire(&framelock);
  if(F_Table==NULL){
    F_Table =(struct framelist *) malloc(sizeof(struct framelist));
    F_Table->upage=upage;
    F_Table->kpage=kpage;
    
  }
  else{
    struct framelist *f = (struct framelist *) malloc(sizeof(struct framelist));
    f->upage=upage;
    f->kpage=kpage;
    f->next=F_Table;
    F_Table=f;
  }
  lock_release(&framelock);
}



void evict(){
  PANIC("OUT OF MEMORY");

}

bool add_mapping(void *upage,void *kpage, bool writable){
  
   struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
   
   bool status= (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
  
  if(status){
    struct page_data *p = malloc(sizeof(struct page_data));
    add_list(kpage,upage);
    p->vaddr=upage;
    p->loc=ram;
    lock_acquire(&SPT_lock);
    SPT_remove(p->vaddr);
    SPT_insert(p);
    lock_release(&SPT_lock);
    
  }
  return status;
}

