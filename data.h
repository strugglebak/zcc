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
extern_ struct Token token_from_file;
extern_ char text_buffer[TEXT_LENGTH + 1];
extern_ struct SymbolTable global_symbol_table[SYMBOL_TABLE_ENTRIES_NUMBER];
extern_ int global_symbol_table_index;

extern_ int current_function_symbol_id;

#endif
