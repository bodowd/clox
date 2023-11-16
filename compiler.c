#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "scanner.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  // there are not exceptions in C, so we use a flag to track whether we're in
  // panic mode or not
  bool panicMode;
} Parser;

// C implicitly gives successively larger numbers for enums, so it means
// PREC_CALL is numerically larger than PREC_UNARY
// if we have an expression: -a.b + c
// if we call parsePrecedence(PREC_ASSIGNMENT), it will parse the entire
// expression because + has a higher precedence than assignment
// if instead we call parsePrecedence(PREC_UNARY), it will compile -a.b and stop
// there it doesn't keep going through the + because the addition has a lower
// precedence than unary operators
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

// function pointer type
typedef void (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compilingChunk;

static Chunk *currentChunk() { return compilingChunk; }

static void errorAt(Token *token, const char *message) {
  // keep compiling as normal as if the error never occurred, and since we are
  // not executing the bytecode, it's ok but while in panic mode, we supress any
  // other errors that get detected
  if (parser.panicMode)
    return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char *message) { errorAt(&parser.previous, message); }

static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    // recall that clox's scanner doesn't report lexical errors. Instead it
    // creates sprecial error tokens and leaves it up to the parser to report
    // them
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;

    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char *message) {
  // like advance in that it reads the next token, but it also validates that
  // the token has an expected type, if not it reports an error
  // This function is the foundation of most syntax errors in the compiler
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitReturn() { emitByte(OP_RETURN); }

static uint8_t makeConstant(Value value) {
  // add value to the constant table
  int constant = addConstant(currentChunk(), value);
  // make sure we do not have too many constants since OP_CONSTANT uses a single
  // byte for the index operand. So we can store only up to 256 constants in a
  // chunk
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}

// emit OP_CONSTANT instruction that pushes it onto the stack at runtime
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}

static void expression();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void grouping() {
  // recursively call back into expression() to comiple the expression between
  // the parentheses, then parse the closing ) at the end
  expression();
  // consume any additional tokens
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void unary() {
  // the leading "-" or "!"  token has been consumed and is sitting in
  // parser.previous
  TokenType operatorType = parser.previous.type;

  // compile the operand
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction
  // we write the negate instruction after its operand's bytecode since "-"
  // or
  // "!" appears on the left. On the stack the operand is on top of the
  // stack above the unary operator. We will evaluate the operand first and
  // leave its value on the stack then we pop that value, negate it, and
  // then push the result. So the OP_NEGATE instruction should be emitted
  // last. Part of the compiler's job is to parse the program in the order
  // it appears in the source code and rearrange it into the order that
  // execution happens
  switch (operatorType) {
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  default:
    return;
  }
}

// binary operators are infix
// With the unary operator, we know what we are parsing from the very first
// token because it starts with "-" or "!"
// with infix expressions, we don't know we're in the middle of a binary
// operator until after we've parsed its left operand and then stumbled onto
// the operator token in the middle
static void binary() {
  TokenType operatorType = parser.previous.type;
  // when we parse the right operand of the * expression in 2*3+4, we need to
  // just capture 3, and not 3+4 because + is lower precedence than *
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emitByte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIVIDE);
    break;
  default:
    return;
  }
}

ParseRule rules[] = {
    // [TOKEN_DOT] = ... syntax is C99's designated initializer syntax.
    // Clearer
    // than counting array indexes by hand
    // prefix func, infix func, precedence
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {NULL, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static void expression() {
  // parse the lowest precedence level, which subsumes all of the
  // higher-precedence expressions too
  parsePrecedence(PREC_ASSIGNMENT);
}

// returns the rule at the given index
// we need this function to handle a declaration cycle in the C code
// binary() is defined before the rules table so that the table can store a
// pointer to it that means the body of binary() cannot access the table
// directly
static ParseRule *getRule(TokenType type) { return &rules[type]; }

// start at the current token and parse any expression at the given precedence
// level or higher
static void parsePrecedence(Precedence precedence) {
  // read the next token and look up the corresponding ParseRule
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  // if no prefix parser, then the token must be a syntax error
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  prefixRule();

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    // with infix we don't know we have a binary operator until we've already
    // parsed the left operand, for example 1+2. We already parsed 1 before we
    // got to the + operator
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
  }
}

bool compile(const char *source, Chunk *chunk) {
  initScanner(source);
  compilingChunk = chunk;
  parser.hadError = false;
  parser.panicMode = false;
  advance();
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  // return false if error occurred (if error, hadError is true, so then !true
  // is false)
  return !parser.hadError;
}
