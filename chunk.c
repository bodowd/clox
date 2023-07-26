#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  // when we initialize a new chunk, also initialize its constant list too
  initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  // also free the constants when the chunk is freed
  freeValueArray(&chunk->constants);
  // call initChunk here to zero out the fields leaving the chunk in a
  // well-defined empty state
  initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
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
    // when the code array grows, we also need to make the line number array
    // grow
    chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
  }

  // store the element and update count
  chunk->code[chunk->count] = byte;
  // store the line number
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

// write the constant value to the chunk's constant pool
int addConstant(Chunk *chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  // return the index where the constant was appended so that we can locate that
  // same constant later
  return chunk->constants.count - 1;
}
