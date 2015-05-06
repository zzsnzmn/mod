#ifndef _MEM_H_
#define _MEM_H_

#include "types.h"

#define ALLOC_FAIL 0xffffffff

// heap memory type
typedef volatile u8 * heap_t;

// setup heap
extern void init_mem(void);
// allocate and return pointer
extern heap_t alloc_mem(u32 bytes);
// free pointer
extern void free_mem(heap_t);

#endif
