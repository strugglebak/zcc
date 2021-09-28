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
extern_ struct SymbolTable symbol_table[SYMBOL_TABLE_ENTRIES_NUMBER];
extern_ int global_symbol_table_index;
extern_ int local_symbol_table_index;

extern_ int current_function_symbol_id;
extern_ int output_dump_ast;
extern_ int output_keep_assembly_file;
extern_ int output_assemble_assembly_file;
extern_ int output_link_object_file;
extern_ int output_verbose;

#endif
