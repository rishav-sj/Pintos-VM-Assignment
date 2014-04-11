#include <stdbool.h>
#include "threads/synch.h"
#include "lib/kernel/hash.h"
/* #include "userprog/process.h" */
#include "filesys/off_t.h"
enum data_location
{
	ram,all_zero,swap,filesys
};


struct page_data
{
  struct hash_elem hash_elem;
  enum data_location loc;
  int swap_sector; //Valid if loc==swap
  void * vaddr;
  struct file* file; //Valid if loc= filesys
  int page_read_bytes; //valid if loc=filesys
  bool writable;
  off_t offset;
};

struct lock SPT_lock;
struct page_data *
SPT_lookup (const void *address);

void SPT_init();

void SPT_insert(struct page_data *p);
bool SPT_remove(const void *address );
