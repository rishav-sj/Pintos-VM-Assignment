/* Code for supplemental page table */
#include <stdbool.h>
#include "lib/kernel/hash.h"
#include "pagetable.h"
#include "threads/synch.h"
struct hash pages;



/* Should be a hashmap for O(1) lookup */
/* Using pseudocode for now */



//HASH : Key : void * , Value: page_data*
//To allow faster lookup in pagefault handler

/**************************************************************************************************************/

/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux )
{
  const struct page_data *p = hash_entry (p_, struct page_data, hash_elem);
  return hash_bytes (&p->vaddr, sizeof p->vaddr);
}

bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux)
{
  const struct page_data *a = hash_entry (a_, struct page_data, hash_elem);
  const struct page_data *b = hash_entry (b_, struct page_data, hash_elem);

  return a->vaddr < b->vaddr;
}

/* Returns the page containing the given virtual address,
   or a null pointer if no such page exists. */
struct page_data *
SPT_lookup (const void *address)
{
  /* printf("Looking up %p \n",address); */
  struct page_data p;
  struct hash_elem *e;

  p.vaddr = address;
  e = hash_find (&pages, &p.hash_elem);
  if( e != NULL ){
    struct page_data* p1= hash_entry (e, struct page_data, hash_elem);
    /* printf("p1 %p \n",p1->vaddr); */
    return p1;
  } 
  else return NULL;
}

void SPT_init(){
  
  hash_init (&pages, page_hash, page_less, NULL);
  lock_init(&SPT_lock);
}

void SPT_insert(struct page_data *p){
 
  /* printf("Insertng address : %p and filesys:%d and ram:  %d \n",p->vaddr,p->loc==filesys,p->loc==ram); */
  hash_insert(&pages,&p->hash_elem);
}

bool SPT_remove(const void *address ){
  /* printf("Rempvogn address %p, \n", address); */
  struct page_data *p=SPT_lookup(address);
  if(p==NULL) return false;
  hash_delete ( &pages, &p->hash_elem);
  return true;
}
