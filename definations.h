struct token {
  int token;
  int int_value;
};

// + - * / [0-9]
enum {
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_INTEGER_LITERAL
};
