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

// 符号表，目前作用是支持变量
struct SymbolTable {
  char *name;
};


// + - * / [0-9] 文件结尾
enum {
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_INTEGER_LITERAL,
  TOKEN_EOF,
  TOKEN_IDENTIFIER, // 标识符
  TOKEN_PRINT, // 关键字 print
  TOKEN_INT, // 关键字 int
  TOKEN_SEMICOLON,// 分号 ;
  TOKEN_EQUALS, // 等号 =
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
