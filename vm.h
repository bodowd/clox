#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  Chunk *chunk;
  // a byte pointer
  // we use a pointer pointing right into the middle of the bytecode array
  // instead of an integer index because it's faster to dereference a pointer
  // than look up an element in an array by index
  uint8_t *ip; // instruction pointer (aka program counter)
  Value stack[STACK_MAX];
  // points to the slot __after__ the top item in the stack
  // in other words, it points to where the next value to be pushed will go
  // we can indicate that the stack is empty by pointing at element zero in the
  // array If pointed to the top element, then for an empty stack, we'd need to
  // point at element -1 which is undefined in C
  Value *stackTop;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();

#endif
