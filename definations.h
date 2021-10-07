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
  struct SymbolTable *symbol_table;
  union {
    int integer_value;
    int scale_size;
  };
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
  union {
    int size; // 在 symbol 中元素的个数
    int end_label; // 对于 STRUCTURAL_FUNCTION 来说的 end label
    int integer_value; // 对于枚举变量来说与其关联的值
  };
  union {
    int position; // 本地变量相对于栈基指针的负向距离
    // 对于函数，为参数的个数
    // 对于结构体，为结构体的 field
    int element_number;
  };
  struct SymbolTable *next; // 下一个 symbol table 的指针
  struct SymbolTable *member; // 指向第一个函数、结构体、联合体、枚举的成员的 symbol table 的指针
  struct SymbolTable *composite_type; // 指向复合类型 symbol table 的指针
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
  TOKEN_STRUCT, // struct

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
  PRIMITIVE_VOID = 16,
  PRIMITIVE_CHAR = 32,
  PRIMITIVE_INT = 48,
  PRIMITIVE_LONG = 64,
  PRIMITIVE_STRUCT = 80,
  PRIMITIVE_UNION,
  PRIMITIVE_ENUM_LIST,
  PRIMITIVE_ENUM_VALUE,
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
  STORAGE_CLASS_FUNCTION_PARAMETER,
  STORAGE_CLASS_STRUCT,
  STORAGE_CLASS_MEMBER,
};

// 如果在 generator.c 中的 interpret_ast_with_register
// 函数没有 register id 返回了，就用这个标志位
enum {
  NO_REGISTER = -1,
  NO_LABEL = 0
};


#define A_OUT "a.out"
#define AS_CMD "as -o"
#define LD_CMD "cc -o"

#endif
