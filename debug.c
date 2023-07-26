#include "debug.h"
#include "chunk.h"
#include "value.h"
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
  printf("instruction: %s\n", name);
  return offset + 1;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant =
      chunk->code[offset + 1]; // pull out the constant index from the
                               // subsequent byte in the chunk -- the index in
                               // array of constants.values
  printf("instruction: %-16s index-of-constant: %4d ", name, constant);
  // look up the actual constant value
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  // return the offset of the beginning of the NEXT instruction -- OP_CONSTANT
  // is two bytes; one for the opcode and one for the operand
  return offset + 2;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  // print the byte offset of the given instruction
  // indicates where in the chunk this instruction is
  printf("offset: %04d ", offset);

  // read a single byte from the bytecode at the given offset to get the opcode
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
