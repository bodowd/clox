#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <stdint.h>

typedef enum {
  OP_RETURN,
} OpCode;

// dynamic array of bytes which holds our code
typedef struct {
  int count;
  int capacity;
  uint8_t *code;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte);

#endif
