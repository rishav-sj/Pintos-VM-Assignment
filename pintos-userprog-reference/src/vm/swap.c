#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "lib/kernel/bitmap.h"
#define SECTORSPERPAGE PGSIZE/BLOCK_SECTOR_SIZE
#include "vm/swap.h"

struct block * swap_block;
int swapsize; //Number of sectors
struct bitmap * occupied; //Map of which sectors are occupied (bit-array)


/* Initially swap block is assumed to be empty*/
/* Size -> number of block sectors */
/* Size of block sector is smaller than size of page */
/* bitmap(bit-array) to maintain which swap sector is free/ which is occupied */




void swap_init()
{
	swap_block = block_get_role(BLOCK_SWAP);
	swapsize = block_size(swap_block);
	occupied = bitmap_create(swapsize);
	lock_init(&swap_lock);
}

/* Writes a page of memory to swap space and correspondingly returns the location at which the page is written*/


int write_page_to_swap(void *kpage)
{
	lock_acquire(&swap_lock);
	/*First find a region with SECTORSPERPAGE sectors free */
	int first_free_loc= bitmap_scan_and_flip (occupied,0,SECTORSPERPAGE,false);
	if (first_free_loc == BITMAP_ERROR)
	{
		PANIC("Out of swap space!");
	}
	else
	{
		/* Write to location */
		/* Have to write in a loop because block can only be written to, one sector at a time */
		int i;
		for (i=0;i<SECTORSPERPAGE;i++)
		{
			block_write(swap_block,first_free_loc+i,kpage+i*BLOCK_SECTOR_SIZE);
		}
	}
	lock_release (&swap_lock);
	return first_free_loc;
}

/*Read a page of memory from the given swap sector 'loc' into the provided buffer 'kpage'*/

void read_page_from_swap(void *kpage,int location)
{

	lock_acquire(&swap_lock);
	int i;
	for (i=0;i<SECTORSPERPAGE;i++)
	{
		block_read(swap_block,location+i,kpage+i*BLOCK_SECTOR_SIZE);
		bitmap_set(occupied,location+i,false);
	}
	lock_release (&swap_lock);

}

void delete_page(int location){
  	lock_acquire(&swap_lock);
	int i;
	for (i=0;i<SECTORSPERPAGE;i++)
	{
	  bitmap_set(occupied,location+i,false);
	}
	lock_release (&swap_lock);
}
