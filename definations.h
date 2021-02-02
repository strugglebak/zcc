#ifndef __DEFINATIONS_H__
#define __DEFINATIONS_H__

struct Token {
  int token;
  int integer_value;
};

struct ASTNode {
  // 操作符  + - * /
  int operation;
  struct ASTNode *left;
  struct ASTNode *right;
  int interger_value;
};

// + - * / [0-9] 文件结尾
enum {
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_INTEGER_LITERAL,
  TOKEN_EOF
};

// AST 节点类型
enum {
  AST_PLUS,
  AST_MINUS,
  AST_MULTIPLY,
  AST_DIVIDE,
  AST_INTEGER_LITERAL,
};

#endif
