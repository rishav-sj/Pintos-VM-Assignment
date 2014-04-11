/* Try to write to the code segment using a system call.
   The process must be terminated with -1 exit code. */

#include "tests/lib.h"
#include "tests/main.h"
#include <stdio.h>

void
test_main (void)
{
  int handle;
  printf("check0\n");
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  printf("check1\n");
  read (handle, (void *) test_main, 1);
  fail ("survived reading data into code segment");
}
