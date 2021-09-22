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
  int storage_class;
  int position; // 本地变量相对于栈基指针的负向距离
};


// + - * / [0-9] 文件结尾
enum {
  TOKEN_EOF,

  // 二元操作符
  TOKEN_ASSIGN, // = 赋值操作
  TOKEN_LOGIC_AND, // &&
  TOKEN_LOGIC_OR, // ||
  TOKEN_OR, TOKEN_XOR, TOKEN_AMPERSAND, // | ^ &
  TOKEN_COMPARE_EQUALS,
  TOKEN_COMPARE_NOT_EQUALS,
  TOKEN_COMPARE_LESS_THAN,
  TOKEN_COMPARE_GREATER_THAN,
  TOKEN_COMPARE_LESS_EQUALS,
  TOKEN_COMPARE_GREATER_EQUALS,
  TOKEN_LEFT_SHIFT, // <<
  TOKEN_RIGHT_SHIFT, // >>
  TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,

  // 其他操作符
  TOKEN_INCREASE, // ++
  TOKEN_DECREASE, // --
  TOKEN_INVERT, // ~
  TOKEN_LOGIC_NOT, // \ !

  // 类型关键字
  TOKEN_VOID, // void
  TOKEN_CHAR, // char
  TOKEN_INT,  // int
  TOKEN_LONG, // long

  // 其他关键字
  TOKEN_IF, // if
  TOKEN_ELSE, // else
  TOKEN_WHILE, // while
  TOKEN_FOR, // for
  TOKEN_RETURN, // return

  // 结构化 token
  TOKEN_INTEGER_LITERAL,
  TOKEN_STRING_LITERAL, // 字符串
  TOKEN_SEMICOLON,// 分号 ;
  TOKEN_IDENTIFIER, // 标识符
  TOKEN_LEFT_BRACE, // {
  TOKEN_RIGHT_BRACE, // }
  TOKEN_LEFT_PAREN, // (
  TOKEN_RIGHT_PAREN, // )
  TOKEN_LEFT_BRACKET, // [
  TOKEN_RIGHT_BRACKET, // ]

  TOKEN_COMMA, // ,
};

// AST 节点类型
enum {
  AST_ASSIGN = 1, // 赋值语句
  AST_LOGIC_OR, AST_LOGIC_AND,
  AST_OR, AST_XOR, AST_AMPERSAND,
  AST_COMPARE_EQUALS,
  AST_COMPARE_NOT_EQUALS,
  AST_COMPARE_LESS_THAN,
  AST_COMPARE_GREATER_THAN,
  AST_COMPARE_LESS_EQUALS,
  AST_COMPARE_GREATER_EQUALS,
  AST_LEFT_SHIFT, AST_RIGHT_SHIFT,
  AST_PLUS, AST_MINUS, AST_MULTIPLY, AST_DIVIDE,

  AST_INTEGER_LITERAL,
  AST_STRING_LITERAL,
  AST_IDENTIFIER, // 普通标识符
  AST_GLUE,

  AST_IF,
  AST_WHILE,
  AST_FUNCTION,
  AST_WIDEN,
  AST_RETURN,

  AST_FUNCTION_CALL,
  AST_DEREFERENCE_POINTER, // 间接引用指针
  AST_IDENTIFIER_ADDRESS, // 指针变量地址
  AST_SCALE,

  AST_PRE_INCREASE,
  AST_PRE_DECREASE,
  AST_POST_INCREASE,
  AST_POST_DECREASE,
  AST_NEGATE,
  AST_INVERT,
  AST_LOGIC_NOT,
  AST_TO_BE_BOOLEAN,

  AST_LVALUE_IDENTIFIER, // 左值
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

enum {
  CENTRAL_GLOBAL = 1,
  CENTRAL_LOCAL,
};

// 如果在 generator.c 中的 interpret_ast_with_register
// 函数没有 register id 返回了，就用这个标志位
#define NO_REGISTER -1

#define NO_LABEL 0

#endif
