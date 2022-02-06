#ifndef __DATA_H__
#define __DATA_H__

#include <stdio.h>
#include "definations.h"

#ifndef extern_
  #define extern_ extern
#endif

#define TEXT_LENGTH 512
#define SYMBOL_TABLE_ENTRIES_NUMBER 1024
#define TOKEN_STRING_NUMBER 128

extern char *token_strings[];
extern_ int line;
extern_ int start_line;
extern_ int putback_buffer;
extern_ FILE *input_file;
extern_ FILE *output_file;
extern_ char *global_output_filename;
extern_ char *global_input_filename;
extern_ struct Token token_from_file;
extern_ struct Token look_ahead_token;
extern_ char text_buffer[TEXT_LENGTH + 1];
extern_ int loop_level;   // 嵌套的循环的层级
extern_ int switch_level; // 嵌套的 switch 的层级

extern_ int output_dump_ast;
extern_ int output_keep_assembly_file;
extern_ int output_assemble_assembly_file;
extern_ int output_link_object_file;
extern_ int output_verbose;
extern_ int output_dump_symbol_table;

extern_ struct SymbolTable *current_function_symbol_id;          // 当前函数
extern_ struct SymbolTable *global_head, *global_tail;           // 全局变量和函数
extern_ struct SymbolTable *local_head, *local_tail;             // 局部变量
extern_ struct SymbolTable *parameter_head, *parameter_tail;     // 局部参数
extern_ struct SymbolTable *composite_head, *composite_tail;     // 复合变量
extern_ struct SymbolTable *temp_member_head, *temp_member_tail; // struct/union 成员的临时 symbol table 指针
extern_ struct SymbolTable *struct_head, *struct_tail;           // struct symbol table 指针
extern_ struct SymbolTable *union_head, *union_tail;             // union symbol table 指针
extern_ struct SymbolTable *enum_head, *enum_tail;               // enum symbol table 指针
extern_ struct SymbolTable *typedef_head, *typedef_tail;         // typedef symbol table 指针


#endif
