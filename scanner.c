#include "scanner.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  // marks the beginning of the _current_ lexeme being scanned
  const char *start;
  // marks the current char being looked at
  const char *current;
  int line;
} Scanner;

// have a global scanner here. easier than passing pointer to all functions that
// need it
Scanner scanner;

void initScanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static bool isAtEnd() { return *scanner.current == '\0'; }

static char advance() {
  // consume the next character and return it
  scanner.current++;
  return scanner.current[-1];
}

// return the current character, but do not consume it
static char peek() { return *scanner.current; }

// like peek but for one character past the current
static char peekNext() {
  if (isAtEnd())
    return '\0';
  return scanner.current[1];
}

// after consuming the first character, we look for an '='
// if found, we consume it and return the corresponding two-character token
// otherwise, leave the current character alone so that it can be part of the
// next token and return the appropriate one-character token
static bool match(char expected) {
  if (isAtEnd())
    return false;
  if (*scanner.current != expected)
    return false;
  // if the current is the desired one, we advance and return true
  scanner.current++;
  return true;
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  // the "lexeme" points to the error message string instead of pointing into
  // the user's source code
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      // when we consume a newline, we also need to bump the line number
      scanner.line++;
      advance();
      break;
    case '/':
      // if we do not find a second /, then skipWhitespace() needs to not
      // consume the first slash either
      if (peekNext() == '/') {
        // a comment goes until the end of the line
        // we check for the new line but do not consume it, this way the newline
        // will be the current character on the next turn of the outer loop
        // in skipWhitespace and we will recognize it and increment
        // scanner.line
        while (peek() != '\n' && !isAtEnd())
          advance();
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {

  // check that the lexeme is exactly as long as the keyword and check that the
  // characters match the rest of a keyword
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  // go through a trie
  switch (scanner.start[0]) {
  case 'a':
    return checkKeyword(1, 2, "nd", TOKEN_AND);
  case 'c':
    return checkKeyword(1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return checkKeyword(1, 3, "lse", TOKEN_ELSE);
  case 'f':
    // check if there is even a letter after 'f'
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return checkKeyword(2, 1, "r", TOKEN_FOR);
      case 'u':
        return checkKeyword(2, 1, "n", TOKEN_FUN);
      }
    }
    break;
  case 'i':
    return checkKeyword(1, 1, "f", TOKEN_IF);
  case 'n':
    return checkKeyword(1, 2, "il", TOKEN_NIL);
  case 'o':
    return checkKeyword(1, 1, "r", TOKEN_OR);
  case 'p':
    return checkKeyword(1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return checkKeyword(1, 4, "uper", TOKEN_SUPER);
  case 't':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'h':
        return checkKeyword(2, 2, "is", TOKEN_THIS);
      case 'r':
        return checkKeyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  case 'v':
    return checkKeyword(1, 2, "ar", TOKEN_VAR);
  case 'w':
    return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) {
    advance();
  }

  return makeToken(identifierType());
}

static Token number() {
  while (isDigit(peek())) {
    advance();
  }

  // look for a fractional part
  if (peek() == '.' && isDigit(peekNext())) {
    // consume the "."
    advance();

    while (isDigit(peek())) {
      advance();
    }
  }

  return makeToken(TOKEN_NUMBER);
}

static Token string() {
  // consume characters unti we reach the closing quote
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      scanner.line++;
    }
    advance();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string.");
  }

  // consume the closing quote
  advance();
  return makeToken(TOKEN_STRING);
}

// since each call to this function scans a complete token, we know we are at
// the beginning of a new token when we enter the function
// so we set scanner.start to the current character so we remember where the
// lexeme we're about to scan starts
Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;

  // check if we are at the end of the source code
  if (isAtEnd())
    return makeToken(TOKEN_EOF);

  char c = advance();

  if (isAlpha(c))
    return identifier();

  if (isDigit(c))
    return number();

  switch (c) {
  case '(':
    return makeToken(TOKEN_LEFT_PAREN);
  case ')':
    return makeToken(TOKEN_RIGHT_PAREN);
  case '{':
    return makeToken(TOKEN_LEFT_BRACE);
  case '}':
    return makeToken(TOKEN_RIGHT_BRACE);
  case ';':
    return makeToken(TOKEN_SEMICOLON);
  case ',':
    return makeToken(TOKEN_COMMA);
  case '.':
    return makeToken(TOKEN_DOT);
  case '-':
    return makeToken(TOKEN_MINUS);
  case '+':
    return makeToken(TOKEN_PLUS);
  case '/':
    return makeToken(TOKEN_SLASH);
  case '*':
    return makeToken(TOKEN_STAR);
  case '!':
    return makeToken(match('=') ? TOKEN_EQUAL : TOKEN_BANG);
  case '=':
    return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return string();
  }

  return errorToken("Unexpected character.");
}
