#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

int find_global_symbol_table_index(char *symbol_string);
int find_local_symbol_table_index(char *symbol_string);
int add_global_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int end_label,
  int size
);
int add_local_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int end_label,
  int size
);
int find_symbol(char *string);

#endif
