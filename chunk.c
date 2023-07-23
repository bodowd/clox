#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  // call initChunk here to zero out the fields leaving the chunk in a
  // well-defined empty state
  initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte) {
  // if the array has reached capactiy we need to
  // 1. allocate a new array with more capacity
  // 2. copy the existing elements from old array to new
  // 3. store the new capacity
  // 4. delete the old array
  // 5. update `code` to pint to the new array
  // 6. store the element in the new array now that there is room
  // 7. update the count
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
  }

  // store the element and update count
  chunk->code[chunk->count] = byte;
  chunk->count++;
}
