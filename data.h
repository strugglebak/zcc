#ifndef __DATA_H__
#define __DATA_H__

#include <stdio.h>
#include "definations.h"

#ifndef extern_
  #define extern_ extern
#endif

#define GET_ARRAY_LENGTH(x)(sizeof(x) / sizeof((x)[0]))
#define TEXT_LENGTH 512
#define SYMBOL_TABLE_ENTRIES_NUMBER 1024

extern_ int line;
extern_ int putback_buffer;
extern_ FILE *input_file;
extern_ FILE *output_file;
extern_ char *global_output_filename;
extern_ struct Token token_from_file;
extern_ char text_buffer[TEXT_LENGTH + 1];

extern_ int output_dump_ast;
extern_ int output_keep_assembly_file;
extern_ int output_assemble_assembly_file;
extern_ int output_link_object_file;
extern_ int output_verbose;

extern_ struct SymbolTable *current_function_symbol_id;          // 当前函数
extern_ struct SymbolTable *global_head, *global_tail;           // 全局变量和函数
extern_ struct SymbolTable *local_head, *local_tail;             // 局部变量
extern_ struct SymbolTable *parameter_head, *parameter_tail;     // 局部参数
extern_ struct SymbolTable *composite_head, *composite_tail;     // 复合变量
extern_ struct SymbolTable *temp_member_head, *temp_member_tail; // struct/union 成员的临时 symbol table 指针
extern_ struct SymbolTable *struct_head, *struct_tail;           // struct symbol table 指针
extern_ struct SymbolTable *union_head, *union_tail;             // union symbol table 指针


#endif
