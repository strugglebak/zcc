#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

struct SymbolTable *find_global_symbol(char *symbol_string);
struct SymbolTable *find_local_symbol(char *symbol_string);
struct SymbolTable *find_composite_symbol(char *symbol_string);
struct SymbolTable *find_temp_member_symbol(char *symbol_string);
struct SymbolTable *find_struct_symbol(char *symbol_string);
struct SymbolTable *find_union_symbol(char *symbol_string);
struct SymbolTable *find_enum_type_symbol(char *symbol_string);
struct SymbolTable *find_enum_value_symbol(char *symbol_string);
struct SymbolTable *find_typedef_symbol(char *symbol_string);
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
  int position,
  struct SymbolTable *composite_type
);
struct SymbolTable *add_global_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class,
  struct SymbolTable *composite_type
);
struct SymbolTable *add_local_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
);
struct SymbolTable *add_parameter_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
);
struct SymbolTable *add_temp_member_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
);
struct SymbolTable *add_struct_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
);
struct SymbolTable *add_union_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
);
struct SymbolTable *add_enum_symbol(
  char *symbol_string,
  int storage_class,
  int value
);
struct SymbolTable *add_typedef_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
);
void clear_all_symbol_tables();
void clear_local_symbol_table();

#endif
