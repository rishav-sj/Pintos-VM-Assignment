/* #define MAX_MEM 0x03ffffff */
/* #define PAGE_SIZE 0x00001000 */
/* #define FRAME_SIZE  MAX_MEM/PAGE_SIZE */
/* #include "threads/palloc.h" */
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <list.h>

struct list framelist;
struct lock framelock;

struct frame{
  struct list_elem elem;
  void *upage;
  void *kpage;
  struct thread *thread;
};

void frame_init();

void evict();

bool add_mapping(void *upage , void *kpage , bool writable,bool setdirty);
void remove_mapping(void *upage,void* kpage,struct list_elem *e,struct thread *t);

