#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include <stdint.h>

typedef enum {
  OP_CONSTANT,
  OP_RETURN,
} OpCode;

// dynamic array of bytes which holds our code
typedef struct {
  int count; // count the number of bytes we have stored
  int capacity;
  uint8_t *code;
  int *lines; // store the line number of each byte in the bytecode for
              // outputting line number when runtime error occurs
  ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);

#endif
