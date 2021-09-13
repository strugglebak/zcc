#ifndef __DEFINATIONS_H__
#define __DEFINATIONS_H__

struct Token {
  int token;
  int integer_value;
};

struct ASTNode {
  // 操作符  + - * /
  int operation;
  int primitive_type; // 对应 void/char/int...
  int rvalue; // boolean, 是否是右值节点，1 为 true, 0 为 false
  struct ASTNode *left;
  struct ASTNode *middle;
  struct ASTNode *right;
  union {
    int interger_value;
    int symbol_table_index;
    int scale_size;
  } value;
};

// 符号表，目前作用是支持变量
struct SymbolTable {
  char *name;         // 每个变量的名字
  int primitive_type; // 每个变量的原始类型
  int structural_type;// 每个变量的结构类型
  int end_label; // 对于 STRUCTURAL_FUNCTION 来说的 end label
  int size; // 在 symbol 中元素的个数
};


// + - * / [0-9] 文件结尾
enum {
  TOKEN_EOF,

  TOKEN_ASSIGN, // = 赋值操作
  TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,

  TOKEN_COMPARE_EQUALS,
  TOKEN_COMPARE_NOT_EQUALS,
  TOKEN_COMPARE_LESS_THAN,
  TOKEN_COMPARE_GREATER_THAN,
  TOKEN_COMPARE_LESS_EQUALS,
  TOKEN_COMPARE_GREATER_EQUALS,

  TOKEN_INTEGER_LITERAL,
  TOKEN_SEMICOLON,// 分号 ;
  TOKEN_EQUALS, // 等号 =
  TOKEN_IDENTIFIER, // 标识符

  TOKEN_LEFT_BRACE, // {
  TOKEN_RIGHT_BRACE, // }
  TOKEN_LEFT_PAREN, // (
  TOKEN_RIGHT_PAREN, // )
  TOKEN_LEFT_BRACKET, // [
  TOKEN_RIGHT_BRACKET, // ]

  TOKEN_PRINT, // 关键字 print
  TOKEN_INT, // 关键字 int

  TOKEN_IF, // if
  TOKEN_ELSE, // else
  TOKEN_WHILE, // while
  TOKEN_FOR, // for

  TOKEN_VOID, // void
  TOKEN_FUNCTION,
  TOKEN_CHAR, // char

  TOKEN_LONG, // long
  TOKEN_RETURN, // return

  TOKEN_AMPERSAND, // &
  TOKEN_LOGICAL_AND, // &&
  TOKEN_COMMA, // ,
};

// AST 节点类型
enum {
  AST_ASSIGNMENT_STATEMENT = 1, // 赋值语句

  AST_PLUS, AST_MINUS, AST_MULTIPLY, AST_DIVIDE,

  AST_COMPARE_EQUALS,
  AST_COMPARE_NOT_EQUALS,
  AST_COMPARE_LESS_THAN,
  AST_COMPARE_GREATER_THAN,
  AST_COMPARE_LESS_EQUALS,
  AST_COMPARE_GREATER_EQUALS,

  AST_INTEGER_LITERAL,
  AST_IDENTIFIER, // 普通标识符

  AST_LVALUE_IDENTIFIER, // 左值

  AST_PRINT,
  AST_GLUE,
  AST_IF,
  AST_WHILE,
  AST_FOR,

  AST_VOID,
  AST_FUNCTION,
  AST_CHAR,
  AST_INT,

  AST_WIDEN,
  AST_SCALE,
  AST_FUNCTION_CALL,
  AST_RETURN,

  AST_DEREFERENCE_POINTER, // 间接引用指针
  AST_IDENTIFIER_ADDRESS, // 指针变量地址
};

// Primitive types 原始类型
enum {
  PRIMITIVE_NONE,
  PRIMITIVE_VOID,
  PRIMITIVE_CHAR,
  PRIMITIVE_INT,
  PRIMITIVE_LONG,
  // 指针类型
  PRIMITIVE_VOID_POINTER,
  PRIMITIVE_CHAR_POINTER,
  PRIMITIVE_INT_POINTER,
  PRIMITIVE_LONG_POINTER,
};

// Structural types 结构类型
enum {
  STRUCTURAL_VARIABLE,
  STRUCTURAL_FUNCTION,
  STRUCTURAL_ARRAY
};

// 如果在 generator.c 中的 interpret_ast_with_register
// 函数没有 register id 返回了，就用这个标志位
#define NO_REGISTER -1

#define NO_LABEL 0

#endif
