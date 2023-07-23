#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

// this macro caculates a new capacity given current capacity
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

// this macro takes care of getting the size of the array's element type and
// casting the resulting void* back to a pointer of the right type
#define GROW_ARRAY(type, pointer, oldCount, newCount)                          \
  (type *)reallocate(pointer, sizeof(type) * (oldCount),                       \
                     sizeof(type) * (newCount))

// deallocate all of the memory (using reallocate to newSize=0)
#define FREE_ARRAY(type, pointer, oldCount)                                    \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif
