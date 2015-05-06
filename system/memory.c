#include <stdio.h>

// asf
#include "print_funcs.h"

// aleph
#include "screen.h"
#include "types.h"
#include "memory.h"


// setup heap-ish
void init_mem(void) {
  // this space left intentionally empty 
}

// allocate and return pointer
heap_t alloc_mem(u32 bytes) {
  void* mem_ptr;

  print_dbg("\r\n >>> alloc_mem(), requested bytes: 0x");
  print_dbg_hex(bytes);

  mem_ptr = malloc(bytes);
  if (mem_ptr == 0) {
     print_dbg("\r\n memory allocation failed!");
  }

  print_dbg("\r\n memory allocation result: 0x");
  print_dbg_hex((u32)mem_ptr);

  return (heap_t)mem_ptr;
}

void free_mem(heap_t mem_ptr)
{
	free((void *)mem_ptr);
}