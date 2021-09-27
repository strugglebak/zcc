#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

int find_global_symbol_table_index(char *symbol_string);
int find_local_symbol_table_index(char *symbol_string);
int add_global_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int end_label,
  int size,
  int storage_class
);
int add_local_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class
);
int find_symbol(char *string);
void reset_local_symbol_index();
void copy_function_parameter(int symbol_table_index);

#endif
