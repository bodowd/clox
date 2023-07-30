#include "debug.h"
#include "chunk.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);
  printf("chunk-count: %d\n", chunk->count);
  for (int offset = 0; offset < chunk->count;) {
    // instead of incrementing offset in the loop, we let
    // disassembleInstruction do it for us by it returning the offset of the
    // next iteration this is useful because instructions can have different
    // sizes
    offset = disassembleInstruction(chunk, offset);
  }
}

static int simpleInstruction(const char *name, int offset) {
  return offset + 1;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant =
      chunk->code[offset + 1]; // pull out the constant index from the
                               // subsequent byte in the chunk -- the index in
                               // array of constants.values
  // look up the actual constant value
  // printValue(chunk->constants.values[constant]);
  // return the offset of the beginning of the NEXT instruction -- OP_CONSTANT
  // is two bytes; one for the opcode and one for the operand
  return offset + 2;
}

// disassemble the long instruction from multiple bytes and back to 4 bytes (32
// bits)
static int longConstantInstruction(const char *name, Chunk *chunk, int offset) {
  // at offset, that's the opcode OP_CONSTANT_LONG
  // offset+1 is where the long constant begins
  uint32_t constant = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8) |
                      (chunk->code[offset + 3] << 16);

  // printValue(chunk->constants.values[constant]);
  return offset + 4;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  // print the byte offset of the given instruction
  // indicates where in the chunk this instruction is
  printf("offset: %04d...", offset);
  int line = getLine(chunk, offset);
  if (offset > 0 && line == getLine(chunk, offset - 1)) {
    // we show | for any instruction that comes from the same source line as the
    // preceding one
    printf("    | ");
  } else {
    printf("line-number: %4d...", line);
  }

  // read a single byte from the bytecode at the given offset to get the opcode
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  case OP_CONSTANT_LONG:
    return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
  case OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
