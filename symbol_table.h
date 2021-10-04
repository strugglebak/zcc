#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

struct SymbolTable *find_global_symbol(char *symbol_string);
struct SymbolTable *find_local_symbol(char *symbol_string);
struct SymbolTable *find_composite_symbol(char *symbol_string);
struct SymbolTable *find_symbol(char *symbol_string);
void append_to_symbol_table(
  struct SymbolTable **head,
  struct SymbolTable **tail,
  struct SymbolTable *node
);

struct SymbolTable *new_symbol_table(
  char *name,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class,
  int position
);
struct SymbolTable *add_global_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class
);
struct SymbolTable *add_local_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class
);
struct SymbolTable *add_parameter_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class
);
void clear_all_symbol_tables();
void clear_local_symbol_table();

#endif
