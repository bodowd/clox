#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lineCount = 0;
  chunk->lineCapacity = 0;
  chunk->lines = NULL;
  // when we initialize a new chunk, also initialize its constant list too
  initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
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
  }

  // store the element and update count
  chunk->code[chunk->count] = byte;
  chunk->count++;

  // see if we are still on the same line
  if (chunk->lineCount > 0 && chunk->lines[chunk->lineCount - 1].line == line) {
    return;
  }

  // grow the lines array if needed
  if (chunk->lineCapacity < chunk->lineCount + 1) {
    int oldCapacity = chunk->lineCapacity;
    chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
    chunk->lines =
        GROW_ARRAY(LineStart, chunk->lines, oldCapacity, chunk->lineCapacity);
  }

  // at this point, we know we aren't on the same line and we have enough
  // room in the lines array
  // add a new LineStart to the lines array and track the offset of the first
  // byte of this line and the line number
  LineStart *lineStart = &chunk->lines[chunk->lineCount++];
  lineStart->offset = chunk->count - 1;
  lineStart->line = line;
}

// write the constant value to the chunk's constant pool
int addConstant(Chunk *chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  // return the index where the constant was appended so that we can locate that
  // same constant later
  return chunk->constants.count - 1;
}

// binary search the lines array to find which LineStart contains the offset we
// want, and thus which line contains that offset
// Since our line numbers monotonically increase, binary search will work
int getLine(Chunk *chunk, int offset) {
  int start = 0;
  int end = chunk->lineCount - 1;

  for (;;) {
    int mid = (start + end) / 2;
    LineStart *line = &chunk->lines[mid];
    if (offset < line->offset) {
      end = mid - 1;
    } else if (mid == chunk->lineCount - 1 ||
               offset < chunk->lines[mid + 1].offset) {
      // if we hit the end of the array or we find that the next offset from the
      // one in LineStart array is bigger than the one we're looking for
      // then this is the closest line to the instruction offset we are looking
      // for
      return line->line;
    } else {
      start = mid + 1;
    }
  }
}

// writes either OP_CONSTANT or OP_CONSTANT_LONG depending on the size of the
// value
// because OP_CONSTANT only uses a single byte for its operand, a chunk could
// only contain up to 256 different constants (2^8)
// here we have another operand if we get past 256 constants
// it stores the operand as a 24-bit number so we have plenty of room for many
// constants
void writeConstant(Chunk *chunk, Value value, int line) {
  // add the constant to the array and get the index back
  // if the index fits in one byte, use the short opcode
  // otherwise, use the long opcode
  // when using the long opcode, need to split the value into multiple bytes
  // to split into multiple bytes, we need to pick an endianness -- here we
  // just go with little-endian as x86 does
  int index = addConstant(chunk, value);
  if (index < 256) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, (uint8_t)index, line);
  } else {
    // each writeChunk will write to the next index in chunk->code array
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    printf("chunk-count: %d", chunk->count);
    // lowest order byte gets written first -- because we & with 0xff (255),
    // we get the first lower order bytes want to & because we then mask all
    // the higher order bytes

    // then we right shift the number by a byte to drop the lower order first
    // byte and then & with 255 to get the bits that are turned on there
    // and so on to the next byte
    writeChunk(chunk, (uint8_t)(index & 0xff), line);
    printf("chunk-count: %d", chunk->count);
    writeChunk(chunk, (uint8_t)((index >> 8) & 0xff), line);
    printf("chunk-count: %d", chunk->count);
    writeChunk(chunk, (uint8_t)((index >> 16) & 0xff), line);
    printf("chunk-count: %d", chunk->count);
  }
}
