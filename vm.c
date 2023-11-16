#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// declare a global VM object instead of passing a pointer to the VM to all the
// functions. We only need one anyway
VM vm;

static void resetStack() { vm.stackTop = vm.stack; }

static void runtimeError(const char *format, ...) {
  // va_list and the ... let us pass an arbitrary number of arguments to this
  // function
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  // the interpreter advances past each instruction before executing it. So to
  // find the failing line we need to look into the current bytecode instruction
  // index minus one
  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction].line;
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

void initVM() { resetStack(); }

void freeVM() {}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  // move from one slot past the last item to the last item and then return
  // that item
  vm.stackTop--;
  return *vm.stackTop;
}

// return a Value from the top of the stack but doesn't pop it
// distance is how far down from the top of the stack to look: zero is the top,
// 1 is one slot down, etc
static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static InterpretResult run() {
// these macros are only used in run, so we define them in run()

// ip advances as soon as we read the opcode, before the instruction has been
// executed b/c ip points to the next byte of code to be used
#define READ_BYTE() (*vm.ip++)
  // reads the next byte from the bytecode, treats the resulting number as an
  // index, and looks up the corresponding Value in the chunk's constant table
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

// do while loop gives us a way to contain multiple statements inside a block
// that also permits a semicolon at the end
// the order of the pops is important
// when the operands themselves are calculated, the left is evaluated first,
// then the right
// that means the left operand gets pushed before the right operand, so the
// right operand will be on the top of the stack, thus the first value we pop is
// b, the right operand
// for example: if we compile 3-1, the instructions look like this:
// push const 3
// push const 1 -- on the top of the stack
// pop 1
// pop 3
// push (3-1)
#define BINARY_OP(valueType, op)                                               \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtimeError("Operands must be numbers.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(valueType(a op b));                                                   \
  } while (false)

  for (;;) {
    // a flag for us to get some diagnostic logging
    // when the flag is defined, the VM disassembles and prints each
    // instruction right before executing it
#ifdef DEBUG_TRACE_EXECUTION
    printf("             ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[");
      printValue(*slot);
      printf("]");
    }
    printf("\n");
    // since disassembleInstruction() takes an integer byte offset
    // and we store the current instruction reference as a direct pointer,
    // we need to convert ip back to a relative offset from the beginning of
    // the bytecode so we take the pointer and subtract it from the pointer
    // where the first byte is to get the offset
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }

    case OP_ADD:
      BINARY_OP(NUMBER_VAL, +);
      break;

    case OP_SUBTRACT:
      BINARY_OP(NUMBER_VAL, -);
      break;

    case OP_MULTIPLY:
      BINARY_OP(NUMBER_VAL, *);
      break;

    case OP_DIVIDE:
      BINARY_OP(NUMBER_VAL, /);
      break;

    case OP_NEGATE:
      // first check if the Value on top of the stack is a number
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      // get the value to operate on with the pop
      // negate the value
      // push it back onto the stack for later instructions
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;

    case OP_RETURN: {
      printValue(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();

  freeChunk(&chunk);
  return result;
}
