#include "userprog/exception.h"
#include "userprog/process.h"
//13018
#include <inttypes.h>
#include <stdio.h>
#include <debug.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "vm/frame.h"
#include "vm/pagetable.h"

#define CODE_SEG 0x08048000
/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));
  
  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

    /* printf ("Page fault at %p: %s error %s page in %s context.\n", */
    /*       fault_addr, */
    /*       not_present ? "not present" : "rights violation", */
    /*       write ? "writing" : "reading", */
    /*       user ? "user" : "kernel"); */
  
 // terminate the process if pagefault
  struct thread *t = thread_current ();
  /* t->exit_status = -1; */
  /* process_terminate(); */

  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  /* printf("do we reach here %p , esp : %p , %d \n ",pg_round_down(fault_addr),t->esp,t->tid); */
  
  if(user) t->esp= f->esp;
  /* printf(" %p \n",t->esp); */
  /* printf(" %p \n",(fault_addr)); */
  struct page_data *p= SPT_lookup(pg_round_down(fault_addr),t);
  
  if(p!=NULL){
    /* printf(" asasas  \n"); */
    switch (p->loc)
      {
      case filesys:
	{
	  size_t page_zero_bytes = PGSIZE - p->page_read_bytes;
	  uint8_t *kpage = palloc_get_page (PAL_USER);
	  int ii=0;
	  while(kpage == NULL){
	    ii++;
	    evict();
	    kpage = palloc_get_page (PAL_USER);
	    /* return false; */
	  }
	 
	  lock_acquire(&filesys_lock);
	  file_seek(p->file,p->offset);
	  /* printf("After file seek \n"); */
	  
	  /* Load this page. */
	  int x=file_read (p->file, kpage,p->page_read_bytes);
	  lock_release(&filesys_lock);
	 
	  if (x != (int)p->page_read_bytes)
	    {
	       if(ii)printf(" i= %d \n",ii);
	        printf("file read %d: %d" ,x,p->page_read_bytes);
	      PANIC("FILE NOT READ");
	      /* printf("After file seek1 \n"); */
	      palloc_free_page (kpage);
	      t->exit_status = -1;
	      process_terminate();
	      return false; 
	    }
	  memset (kpage + p->page_read_bytes, 0, page_zero_bytes);
	  /* Add the page to the process's address space. */
	  /* printf("After file seek2 \n"); */
	  /* printf("writeable: %d \n",p->writable); */
	  if (!add_mapping (p->vaddr, kpage, p->writable,false)) 
	    {
	      printf("After file seek3 \n");
	      palloc_free_page (kpage);
	      t->exit_status = -1;
	      process_terminate();
	     
	      return false; 
	    }
	  
	  return;
	}
	break;
      case mmap1:
	{
	  /* printf("After mmap \n"); */
	  void *kpage= palloc_get_page(PAL_USER);
	  while (kpage == NULL){
	    evict();
	    kpage = palloc_get_page (PAL_USER);
	    /* if(kpage==NULL) */
	      /* PANIC("kpage still NULL \n"); */
	    /* return false; */
	  }
	  /* int fd=p->fd; */
	  /* struct thread *cur = thread_current (); */
	  struct file* fi =p->file;
	  /* printf("check1  %d\n",p->offset); */
	  file_read_at(fi,kpage,p->length,p->offset);
	  /* printf("kapge : %s  %d  %d  \n",kpage,p->length,p->offset); */
	  /* printf("check2 \n");	  */
	  if (!add_mapping (p->vaddr, kpage,1,false)) 
	    {
	      PANIC("Couldnt add mapping in Mmap");
	      palloc_free_page (kpage);
	      return false; 
	    }
	  return;
	}
	break;
      case swap:
	{
	  /* printf("loading back swap  %p %p\n",p->vaddr,fault_addr); */
	    /* lock_acquire(&frame_lock); */
	  void *kpage= palloc_get_page(PAL_USER);
	  while(kpage == NULL){
	    evict();
	    kpage = palloc_get_page (PAL_USER);
	    /* if(kpage==NULL) */
	      /* PANIC("kpage still NULL \n"); */
	    /* return false; */
	  }
	  
	  /* lock_release(&frame_lock); */
	  bool test=add_mapping (p->vaddr, kpage,1,true);
	   if (!test) 
	    {
	      PANIC("Couldnt add mapping in swap");
	      palloc_free_page (kpage);    
	      return false; 
	    }
	   /* else */
	     /* pagedir_set_dirty(t->pagedir,p->vaddr,false); */
	  read_page_from_swap(kpage,p->block_sector);
	  SPT_remove(p,t);
	  return;
	}
	break;
      default:
	{
	  PANIC("NOT CASE-INCORRECT LOC");
	  t->exit_status = -1; 
	  process_terminate();
	}
      }
  }
  
  printf(" we soundt be here  %d\n",t->tid);
  if( !( t->esp-fault_addr< (void *)128 || fault_addr-t->esp < (void*) 128))
    {
   t->exit_status = -1;
    process_terminate();
  }   
  if((!fault_addr) ||  (fault_addr >= (void*)PHYS_BASE) ) {
    printf("term1\n");
    PANIC("RUN");
    t->exit_status = -1;
    process_terminate();
  }
  if(fault_addr>=CODE_SEG && fault_addr <= ( CODE_SEG+t->filesz)){
    printf("Code segment access \n");
    t->exit_status = -1;
    process_terminate();
  }
 
    /* if(p->loc!=filesys){ */
    /*   /\* printf("term0\n"); *\/ */
    /*   t->exit_status = -1;  */
    /* 	process_terminate(); */
    /* } //remove later */
   
  /* printf("gelp \n "); */
  
     

  /* printf ("Page fault at %p: %s error %s page in %s context and %p.\n", */
  /*         fault_addr, */
  /*         not_present ? "not present" : "rights violation", */
  /*         write ? "writing" : "reading", */
  /*         user ? "user" : "kernel", */
  /* 	  t->esp); */
  
  
  /* printf("new %x: \n", ((t->esp)-(fault_addr))-0x20 ); */
  if((mod((t->esp)-(fault_addr))<= 0x20) && (t->numpages<MAX_STACK)){
    uint8_t *kpage;
    bool success = false;
  
    kpage = palloc_get_page (PAL_USER | PAL_ZERO);
    while( kpage==NULL){
      evict();
      kpage=palloc_get_page (PAL_USER | PAL_ZERO);
    }
    if (kpage != NULL) 
      {
	success = add_mapping (((uint8_t *) PHYS_BASE) -(t->numpages+1)*PGSIZE, kpage, true,false);
	if (success){
	  //*esp = PHYS_BASE;
	  t->numpages++;
	}
	else
	  palloc_free_page (kpage);
      }
  }
  else {
    t->exit_status = -1;
    process_terminate();
    kill(f);
  }

  
}

int mod( int x){
  return x>0?x:-x;
}

