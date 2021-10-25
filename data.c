#include "data.h"

char *token_string[TOKEN_STRING_NUMBER] = {
  "EOF", "=", "+=", "-=", "*=", "/=", "?",
  "&&", "||", "|", "^", "&",
  "==", "!=", ",", ">", "<=", ">=", "<<", ">>",
  "+", "-", "*", "/", "++", "--", "~", "!",
  "void", "char", "int", "long",
  "if", "else", "while", "for", "return",
  "struct", "union", "enum", "typedef",
  "extern", "break", "continue", "switch",
  "case", "default", "sizeof", "static",
  "integer_literal", "string_literal", ";", "identifier",
  "{", "}", "(", ")", "[", "]", ",", ".",
  "->", ":"
};
