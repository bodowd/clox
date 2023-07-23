#include "memory.h"
#include <stdlib.h>

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
  // cases to handle:

  // oldSize, newSize, operation
  // ---------------------------
  // 0, non-zero, allocate new block
  // non-zero, 0, free allocation
  // non-zero, smaller than oldSize, shrink existing allocation
  // non-zero, larget than oldSize, grow existing allocation

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  // realloc conveniently supports the other three aspects of our policy
  // when oldSize is zero, realloc is equivalent to malloc
  void *result = realloc(pointer, newSize);

  // if there isn't enough memory, realloc will return NULL
  if (result == NULL)
    exit(1);
  return result;
}
