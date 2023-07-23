#include "debug.h"
#include "chunk.h"
#include <stdint.h>
#include <stdio.h>

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    // instead of incrementing offset in the loop, we let disassembleInstruction
    // do it for us by it returning the offset of the next iteration
    // this is useful because instructions can have different sizes
    offset = disassembleInstruction(chunk, offset);
  }
}

static int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  // print the byte offset of the given instruction
  // indicates where in the chunk this instruction is
  printf("%04d ", offset);

  // read a single byte from the bytecode at the given offset to get the opcode
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
