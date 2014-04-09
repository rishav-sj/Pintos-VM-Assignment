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
void frame_init();
void add_list(void *kapge,void *upage);
void evict();

bool add_mapping(void *upage , void *kpage , bool writable);
