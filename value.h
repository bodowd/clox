#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// a tagged union
// value contains two parts: a type "tag" and a paylod for the actual value
// below is the kind of value the VM supports -- the VM's notion of type, not
// necessarily the user's notion of type
// for example, every instance of a class is just type "instance" to the VM
typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} ValueType;

// here is the tagged union
// a union looks like a struct except that all of its fields overlap in memory
// in a struct, you callocate memory for each field
// i.e. 8-byte double + 1-byte bool
// with the union, it overlaps in memory. That helps us save memory here because
// a value cannot simultaneously be a number and a boolean at the same time
// so at any point in time, only one of those fields will be used.
// the size of a union is the size of its largest field
// so with 8-byte double + 1 byte bool, the union will be 8-bytes total, but
// reuse the first byte for example if it is a bool (vs 9byte if it were a
// struct)
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as;
} Value;

// anytime we call one of the AS_ macros, we need to guard it behind a call to
// one of these IS_ macros first. This helps keep us from accessing the wrong
// memory in the union
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

// constant pool is a dynamic array of values
typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif
