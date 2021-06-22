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
  struct ASTNode *root;
  struct ASTNode *right;
  union {
    int interger_value;
    int symbol_table_index;
  } value;
};

// 符号表，目前作用是支持变量
struct SymbolTable {
  char *name;
};


// + - * / [0-9] 文件结尾
enum {
  TOKEN_EOF,

  TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,

  TOKEN_COMPARE_EQUALS,
  TOKEN_COMPARE_NOT_EQUALS,
  TOKEN_COMPARE_LESS_THAN,
  TOKEN_COMPARE_GREATER_THAN,
  TOKEN_COMPARE_LESS_EQUALS,
  TOKEN_COMPARE_GREATER_EQUALS,

  TOKEN_INTEGER_LITERAL,
  TOKEN_IDENTIFIER, // 标识符
  TOKEN_SEMICOLON,// 分号 ;
  TOKEN_EQUALS, // 等号 =

  TOKEN_PRINT, // 关键字 print
  TOKEN_INT, // 关键字 int

  TOKEN_LEFT_BRACE, // {
  TOKEN_RIGHT_BRACE, // }
  TOKEN_LEFT_PAREN, // (
  TOKEN_RIGHT_PAREN, // )

  TOKEN_IF, // if
  TOKEN_ELSE // else
};

// AST 节点类型
enum {
  AST_PLUS = 1, AST_MINUS, AST_MULTIPLY, AST_DIVIDE,

  AST_COMPARE_EQUALS,
  AST_COMPARE_NOT_EQUALS,
  AST_COMPARE_LESS_THAN,
  AST_COMPARE_GREATER_THAN,
  AST_COMPARE_LESS_EQUALS,
  AST_COMPARE_GREATER_EQUALS,

  AST_INTEGER_LITERAL,
  AST_IDENTIFIER, // 普通标识符

  AST_LVALUE_IDENTIFIER, // 左值
  AST_ASSIGNMENT_STATEMENT, // 赋值语句

  AST_GLUE
};

#endif
