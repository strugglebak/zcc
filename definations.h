#ifndef __DEFINATIONS_H__
#define __DEFINATIONS_H__

#include "incdir.h"

#define A_OUT "a.out"
#define AS_CMD "as -o "
#define LD_CMD "cc -o "
#define CPP_CMD "cpp -nostdinc -isystem "

enum {
  TOKEN_EOF,

  // 二元操作符
  TOKEN_ASSIGN, // = 赋值操作
  // +=, -=, *=, /=
  TOKEN_ASSIGN_PLUS, TOKEN_ASSIGN_MINUS, TOKEN_ASSIGN_MULTIPLY, TOKEN_ASSGIN_DIVIDE, TOKEN_ASSGIN_MOD,
  TOKEN_QUESTION, // \?
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
  TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_MOD,

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
  TOKEN_IF,       // if
  TOKEN_ELSE,     // else
  TOKEN_WHILE,    // while
  TOKEN_FOR,      // for
  TOKEN_RETURN,   // return
  TOKEN_STRUCT,   // struct
  TOKEN_UNION,    // union
  TOKEN_ENUM,     // enum
  TOKEN_TYPEDEF,  // typedef
  TOKEN_EXTERN,   // extern
  TOKEN_BREAK,    // break
  TOKEN_CONTINUE, // continue
  TOKEN_SWITCH,   // switch
  TOKEN_CASE,     // case
  TOKEN_DEFAULT,  // default
  TOKEN_SIZEOF,   // sizeof
  TOKEN_STATIC,   // static

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
  TOKEN_DOT, // .
  TOKEN_ARROW, // ->
  TOKEN_COLON // :
};

struct Token {
  int token;
  int integer_value;
  char *token_string;
};

// AST 节点类型
enum {
  AST_ASSIGN = 1, // 赋值语句
  AST_ASSIGN_PLUS, AST_ASSIGN_MINUS, AST_ASSIGN_MULTIPLY, AST_ASSIGN_DIVIDE, AST_ASSIGN_MOD,
  AST_TERNARY, // 三元操作符
  AST_LOGIC_AND, AST_LOGIC_OR,
  AST_OR, AST_XOR, AST_AMPERSAND,
  AST_COMPARE_EQUALS,
  AST_COMPARE_NOT_EQUALS,
  AST_COMPARE_LESS_THAN,
  AST_COMPARE_GREATER_THAN,
  AST_COMPARE_LESS_EQUALS,
  AST_COMPARE_GREATER_EQUALS,
  AST_LEFT_SHIFT, AST_RIGHT_SHIFT,
  AST_PLUS, AST_MINUS, AST_MULTIPLY, AST_DIVIDE, AST_MOD,

  AST_INTEGER_LITERAL,
  AST_STRING_LITERAL,
  AST_IDENTIFIER, // 普通标识符
  AST_GLUE,

  AST_IF,
  AST_WHILE,
  AST_FUNCTION,
  AST_WIDEN,
  AST_RETURN,
  AST_BREAK,
  AST_CONTINUE,
  AST_SWITCH,
  AST_CASE,
  AST_DEFAULT,
  AST_TYPE_CASTING, // 类型强制转换

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
  AST_TO_BE_BOOLEAN
};

// Primitive types 原始类型
enum {
  PRIMITIVE_NONE,
  PRIMITIVE_VOID = 16,
  PRIMITIVE_CHAR = 32,
  PRIMITIVE_INT = 48,
  PRIMITIVE_LONG = 64,
  PRIMITIVE_STRUCT = 80,
  PRIMITIVE_UNION = 96
};

// Structural types 结构类型
enum {
  STRUCTURAL_VARIABLE,
  STRUCTURAL_FUNCTION,
  STRUCTURAL_ARRAY
};

enum {
  STORAGE_CLASS_GLOBAL = 1,
  STORAGE_CLASS_LOCAL,
  STORAGE_CLASS_EXTERN,
  STORAGE_CLASS_STATIC,
  STORAGE_CLASS_FUNCTION_PARAMETER,
  STORAGE_CLASS_STRUCT,
  STORAGE_CLASS_UNION,
  STORAGE_CLASS_MEMBER,
  STORAGE_CLASS_ENUM_TYPE,
  STORAGE_CLASS_ENUM_VALUE,
  STORAGE_CLASS_TYPEDEF
};

// 符号表，目前作用是支持变量
// 组成单向链表
// 1. 全局变量以及函数(global variable/function)
// 2. 对于当前函数来说的局部变量(local variable)
// 3. 对于当前函数来说的局部参数(local parameter)
// 4. 定义的结构体(struct)
// 5. 定义的联合体(union)
// 6. 定义的枚举以及其对应的值(enum)
struct SymbolTable {
  char *name;         // 每个变量的名字
  int primitive_type; // 每个变量的原始类型
  int structural_type;// 每个变量的结构类型
  int storage_class;
  int size; // 在 symbol 中元素的个数

  // 对于函数，为参数的个数
  // 对于结构体，为结构体的 field 的个数
  int element_number;

// 对于 STRUCTURAL_FUNCTION 来说的 end label
#define symbol_table_end_label symbol_table_position

  // 本地变量相对于栈基指针的负向距离
  int symbol_table_position;

  int *init_value_list; // 初始化值列表
  struct SymbolTable *next; // 下一个 symbol table 的指针
  struct SymbolTable *member; // 指向第一个函数参数、第一个 struct/union/enum 的 member 成员的 symbol table 指针
  struct SymbolTable *composite_type; // 指向复合类型 symbol table 的指针
};

struct ASTNode {
  // 操作符  + - * /
  int operation;
  int primitive_type; // 对应 void/char/int...
  int rvalue; // boolean, 是否是右值节点，1 为 true, 0 为 false
  struct ASTNode *left;
  struct ASTNode *middle;
  struct ASTNode *right;
  struct SymbolTable *symbol_table;
  struct SymbolTable *composite_type; // 指向复合类型 symbol table 的指针

#define ast_node_integer_value ast_node_scale_size
int ast_node_scale_size;
};

// 如果在 generator.c 中的 interpret_ast_with_register
// 函数没有 register id 返回了，就用这个标志位
enum {
  NO_REGISTER = -1,
  NO_LABEL = 0
};

#endif
