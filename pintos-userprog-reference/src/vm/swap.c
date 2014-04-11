#include "devices/block.h"
#include "threads/vaddr.h"

#define SECTORSPERPAGE PGSIZE/BLOCK_SECTOR_SIZE
struct block * swap_block;
int swapsize; //Number of sectors
bitmap * occupied; //Map of which sectors are occupied (bit-array)

/* Initially swap block is assumed to be empty*/
/* Size -> number of block sectors */
/* Size of block sector is smaller than size of page */
/* bitmap(bit-array) to maintain which swap sector is free/ which is occupied */

void swap_init()
{
	swap_block = block_get_role(BLOCK_SWAP);
	
	swapsize = swap_block->size;

	occupied = bitmap_create(swapsize);
}


void write_page_to_swap(void *kpage)
{

	/*First find a region with SECTORSPERPAGE sectors free */

	int first_free_loc= bitmap_scan_and_flip (occupied,0,SECTORSPERPAGE,false);

	if (first_free_loc == BITMAP_ERROR)
	{
		PANIC("Sorry! Out of swap space!");
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
}







