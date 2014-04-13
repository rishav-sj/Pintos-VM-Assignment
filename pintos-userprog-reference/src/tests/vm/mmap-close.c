/* Verifies that memory mappings persist after file close. */

#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/arc4.h"
#include "tests/lib.h"
#include "tests/main.h"

#define ACTUAL ((void *) 0x10000000)

void
test_main (void)
{
  int handle;
  mapid_t map;

  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  CHECK ((map = mmap (handle, ACTUAL)) != MAP_FAILED, "mmap \"sample.txt\"");
  /* printf("before handle \n"); */
  close (handle); 

  if (memcmp (ACTUAL, sample, strlen (sample)))
    fail ("read of mmap'd file reported bad data");
  /* printf("before handle1 \n");  */
  /* printf("before handle2 \n"); */
  munmap (map);
   /* printf("before handle3 \n"); */
}
