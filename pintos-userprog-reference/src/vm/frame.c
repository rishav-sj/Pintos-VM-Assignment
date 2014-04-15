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





struct list_elem* first_clean();
void frame_init(){
  lock_init(&framelock);
    /* SPT_init(); */
  lock_init(&filesys_lock2);
  list_init (&framelist);
  swap_init();
}

void add_list(void *kpage,void *upage){
  /* lock_acquire(&framelock); */
  struct frame *f = malloc(sizeof(struct frame));
  f->thread=thread_current();
  f->upage=upage;
  f->kpage=kpage;   
  list_push_back( &framelist, &f->elem);
  /* lock_release(&framelock); */
}

/* void remove(void *upage,struct list_elem *e){ */
/*   /\* struct frame *f =  malloc(sizeof(struct frame)); *\/ */
/*   /\* f->upage=upage; *\/ */
/*   /\* list_remove (&f->elem); *\/ */
/*   /\* struct list_elem *e; *\/ */
/*   /\* printf("dowe cal this11 \n"); *\/ */

/*   /\* for (e = list_begin (&framelist); e != list_end (&framelist); *\/ */
/*   /\*      e = list_next (e)) *\/ */
/*   /\*   { *\/ */
/*   /\*     struct frame *f = list_entry(e, struct frame , elem); *\/ */
/*   /\*     	  /\\* printf(" remove %p %p \n ",f->upage,f->kpage); *\\/ *\/ */
/*   /\*     if(f->upage==upage){ *\/ */
/* 	/\* if(is_interior(e)) *\/ */
/* 	  list_remove(e); */
/*     /\* 	  printf("do we cal this \n"); *\/ */
/*     /\* 	  break; *\/ */
/*     /\*   } *\/ */
/*     /\*   /\\* elseif(is_head(e)) *\\/ *\/ */

/*     /\* } *\/ */

/* } */

void evict(){
 
  /* struct list_elem* e= first_clean(); */
  lock_acquire(&framelock);
  struct list_elem* e= list_front(&framelist);
  struct frame *f = list_entry(e, struct frame , elem);
  
  if (pagedir_is_dirty(f->thread->pagedir,f->upage))
  /* if(e==NULL) */
  {
     
      ASSERT(f!=NULL);
      
      /* printf("sector %d \n",sector); */
      struct page_data *p = SPT_lookup(f->upage,f->thread);
      if(p!=NULL){
	
	if(p->loc==mmap1)
	  {
	    printf("removing2\n");
		  write_back_map(f->upage,p->file,p->offset);	
		  remove_mapping(f->upage,f->kpage,e,f->thread); 
		   lock_release(&framelock);
		   return;
	  }
	else
	  SPT_remove(p->vaddr,f->thread);
      }
      int sector= write_page_to_swap(f->kpage);
      /* printf("evicton 4 \n"); */
      struct page_data *p1= malloc(sizeof(struct page_data));
      p1->loc= swap;
      p1->vaddr=f->upage;
      p1->block_sector=sector;
      SPT_insert(p1,f->thread);

      /* printf("evicton 5 \n"); */
      /* printf("in evic3 %p , %p thread: %d\n", f->upage,f->kpage,f->thread->tid); */
      remove_mapping(f->upage,f->kpage,e,f->thread); 
    }
  else
    {
 
      remove_mapping(f->upage,f->kpage,e,f->thread); 
    }
  lock_release(&framelock);
  /* PANIC("OUT OF MEMORY"); */
}

bool add_mapping(void *upage,void *kpage, bool writable,bool setdirty){
  struct thread *t = thread_current ();
  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  lock_acquire(&framelock);
  bool status= (pagedir_get_page (t->pagedir, upage) == NULL
		&& pagedir_set_page (t->pagedir, upage, kpage, writable));
  if(status){
    if(setdirty) pagedir_set_dirty(t->pagedir,upage,true);
    add_list(kpage,upage);
  }
  lock_release(&framelock);
  return status;
  
}
void remove_mapping(void *upage, void *kpage, struct list_elem *e,struct thread *t){
  /* printf("bool %d \n",list_empty(&framelist)); */
  list_remove(e);
  /* printf("list size %d \n",list_size(&framelist)); */
 
  /* printf("are we here %p ]\n" ,upage); */
  pagedir_clear_page(t->pagedir,upage);
  palloc_free_page(kpage);
}

struct list_elem* first_clean(){
  
      struct list_elem *e;

      for (e = list_rbegin (&framelist); e != list_rend (&framelist);
           e = list_prev(e))
        {
          struct frame *f = list_entry(e, struct frame , elem);
	  /* printf(" first clean %p %p \n ",f->upage,f->kpage); */
	  if(!pagedir_is_dirty(thread_current()->pagedir,f->upage) && f->thread==thread_current())
	    return e;
	  
        }
      return NULL;
}

struct list_elem* get_elem(void * upage){
  
      struct list_elem *e;

      for (e = list_begin (&framelist); e != list_end (&framelist);
           e = list_next (e))
        {
          struct frame *f = list_entry(e, struct frame , elem);
	  /* printf(" first clean %p %p \n ",f->upage,f->kpage); */
	  if(f->upage==upage)
	    return e;
	  
        }
      return NULL;
}
