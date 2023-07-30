#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include <stdint.h>

typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_CONSTANT_LONG,
  OP_NEGATE,
  OP_RETURN,
} OpCode;

// used to mark the start of a new line in the source code
typedef struct {
  int offset; // mark the offset of the first instruction on that line
  int line;   // track which line that instruction is on
} LineStart;

// dynamic array of bytes which holds our code
typedef struct {
  int count; // count the number of bytes we have stored
  int capacity;
  uint8_t *code;
  ValueArray constants;
  int lineCount; // count how much is in lines since it will be different than
                 // code because we will only increase lines when there is a
                 // new line
  int lineCapacity;
  LineStart *lines;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
int getLine(Chunk *chunk, int offset);
void writeConstant(Chunk *chunk, Value value, int line);

#endif
