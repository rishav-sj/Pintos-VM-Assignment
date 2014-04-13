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
  list_init (&framelist);
  swap_init();
}

void add_list(void *kpage,void *upage){
  lock_acquire(&framelock);
  struct frame *f = malloc(sizeof(struct frame));
  f->thread=thread_current();
  f->upage=upage;
  f->kpage=kpage;   
  list_push_back( &framelist, &f->elem);
  lock_release(&framelock);
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
  /* printf("in evic1\n"); */
  /* struct list_elem* e= first_clean(); */
  struct list_elem* e= list_front(&framelist);
  struct frame *f = list_entry(e, struct frame , elem);
  
  if (pagedir_is_dirty(thread_current()->pagedir,f->upage))
    {
     
      ASSERT(f!=NULL);
      
      /* printf("sector %d \n",sector); */
      struct page_data *p = SPT_lookup(f->upage);
      if(p!=NULL){
	/* printf("removing2\n"); */
	if(p->loc==mmap1)
	  {
		  write_back_map(f->upage,p->file,p->offset);	
		  return;
	  }
	else
	SPT_remove(p->vaddr);
      }
      int sector= write_page_to_swap(f->kpage);
      /* printf("evicton 4 \n"); */
      struct page_data *p1= malloc(sizeof(struct page_data));
      p1->loc= swap;
      p1->vaddr=f->upage;
      p1->block_sector=sector;
      SPT_insert(p1);
      /* printf("evicton 5 \n"); */
      remove_mapping(f->upage,f->kpage,e); 
    }
  else
    {
      /* printf("in evic3 %p , %p\n", f->upage,f->kpage); */
      remove_mapping(f->upage,f->kpage,e); 
    }
  /* PANIC("OUT OF MEMORY"); */
}

bool add_mapping(void *upage,void *kpage, bool writable){
  struct thread *t = thread_current ();
  /* Verify that there's not already a page at that virtual
     address, then map our page there. */

  bool status= (pagedir_get_page (t->pagedir, upage) == NULL
		&& pagedir_set_page (t->pagedir, upage, kpage, writable));
  if(status){
    add_list(kpage,upage);
  }

  return status;

}
void remove_mapping(void *upage, void *kpage, struct list_elem *e){
  /* remove(upage); */
  /* printf("list size before %d \n",list_size(&framelist)); */
  /* printf("bool %d \n",list_empty(&framelist)); */
  list_remove(e);
  /* printf("list size %d \n",list_size(&framelist)); */
  struct thread *t = thread_current ();
  /* printf("are we here %p ]\n" ,upage); */
  pagedir_clear_page(t->pagedir,upage);
    /* printf("in remove1\n"); */
  palloc_free_page(kpage);
    /* printf("in remove2\n"); */
}

struct list_elem* first_clean(){
  
      struct list_elem *e;

      for (e = list_begin (&framelist); e != list_end (&framelist);
           e = list_next (e))
        {
          struct frame *f = list_entry(e, struct frame , elem);
	  /* printf(" first clean %p %p \n ",f->upage,f->kpage); */
	  if(!pagedir_is_dirty(thread_current()->pagedir,f->upage))
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
