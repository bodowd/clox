#include "chunk.h"
#include "common.h"
#include "debug.h"

int main(int argc, const char *argv[]) {
  Chunk chunk;
  initChunk(&chunk);
  int constant = addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);
  writeConstant(&chunk, 20, 125);
  writeChunk(&chunk, OP_RETURN, 126);
  disassembleChunk(&chunk, "test chunk");
  freeChunk(&chunk);
  return 0;
}
