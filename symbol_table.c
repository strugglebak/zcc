#include <string.h>
#include <stdlib.h>
#include "helper.h"
#include "data.h"
#include "generator.h"
#include "symbol_table.h"
#include "definations.h"

static struct SymbolTable *add_symbol_core(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class,
  int position,
  struct SymbolTable** head,
  struct SymbolTable** tail,
  struct SymbolTable *composite_type
) {
  struct SymbolTable *t = new_symbol_table(
    symbol_string,
    primitive_type,
    structural_type,
    size,
    storage_class,
    position,
    composite_type
  );
  append_to_symbol_table(head, tail, t);
  return t;
}

static struct SymbolTable *find_symbol_in_list(
  char *symbol_string,
  struct SymbolTable *list,
  int storage_class
) {
  for (; list; list = list->next) {
    if (list->name && (!strcmp(symbol_string, list->name)))
      if (!storage_class || storage_class == list->storage_class)
        return list;
  }
  return NULL;
}

struct SymbolTable *find_global_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, global_head, 0);
}

struct SymbolTable *find_local_symbol(char *symbol_string) {
  struct SymbolTable *node;
  // 优先判断是否在函数体内，如果是则寻找 parameter
  if (current_function_symbol_id) {
    node = find_symbol_in_list(symbol_string, current_function_symbol_id->member, 0);
    if (node) return node;
  }
  return find_symbol_in_list(symbol_string, local_head, 0);
}

struct SymbolTable *find_composite_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, composite_head, 0);
}

struct SymbolTable *find_temp_member_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, temp_member_head, 0);
}

struct SymbolTable *find_struct_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, struct_head, 0);
}

struct SymbolTable *find_union_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, union_head, 0);
}

struct SymbolTable *find_enum_type_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, enum_head, STORAGE_CLASS_ENUM_TYPE);
}

struct SymbolTable *find_enum_value_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, enum_head, STORAGE_CLASS_ENUM_VALUE);
}

struct SymbolTable *find_typedef_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, typedef_head, 0);
}

struct SymbolTable *find_symbol(char *symbol_string) {
  struct SymbolTable *node = find_local_symbol(symbol_string);
  if (node) return node;
  return find_global_symbol(symbol_string);
}

void append_to_symbol_table(
  struct SymbolTable **head,
  struct SymbolTable **tail,
  struct SymbolTable *node
) {
  if (!head || !tail || !node)
    error("Either head, tail or node is NULL in append_to_symbol_table");

  if (*tail) {
    (*tail)->next = node;
    *tail = node;
  } else
    *head = *tail = node;

  node->next = NULL;
}

/**
 * new 一个新的 symbol table
*/
struct SymbolTable *new_symbol_table(
  char *name,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class,
  int position,
  struct SymbolTable *composite_type
) {
  struct SymbolTable *node = (struct SymbolTable *) malloc(sizeof(struct SymbolTable));
  if (!node)
    error("Unable to malloc a symbol table node in update_symbol_table");

  node->name = strdup(name);
  node->primitive_type = primitive_type;
  node->structural_type = structural_type;
  node->size = size;
  node->storage_class = storage_class;
  node->position = position;
  node->composite_type = composite_type;
  node->next = NULL;
  node->member = NULL;

  if (storage_class == STORAGE_CLASS_GLOBAL)
    generate_global_symbol(node);

  return node;
}
struct SymbolTable *add_global_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  int storage_class,
  struct SymbolTable *composite_type
) {
  return add_symbol_core(
    symbol_string,
    primitive_type,
    structural_type,
    size,
    storage_class,
    0,
    &global_head,
    &global_tail,
    composite_type
  );
}
struct SymbolTable *add_local_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
) {
  return add_symbol_core(
    symbol_string,
    primitive_type,
    structural_type,
    size,
    STORAGE_CLASS_LOCAL,
    0,
    &local_head,
    &local_tail,
    composite_type
  );
}

struct SymbolTable *add_parameter_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  struct SymbolTable *composite_type
) {
  return add_symbol_core(
    symbol_string,
    primitive_type,
    structural_type,
    1,
    STORAGE_CLASS_FUNCTION_PARAMETER,
    0,
    &parameter_head,
    &parameter_tail,
    composite_type
  );
}

struct SymbolTable *add_temp_member_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
) {
  return add_symbol_core(
    symbol_string,
    primitive_type,
    structural_type,
    size,
    STORAGE_CLASS_MEMBER,
    0,
    &temp_member_head,
    &temp_member_tail,
    composite_type
  );
}
struct SymbolTable *add_struct_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
) {
  return add_symbol_core(
    symbol_string,
    primitive_type,
    structural_type,
    size,
    STORAGE_CLASS_STRUCT,
    0,
    &struct_head,
    &struct_tail,
    composite_type
  );
}
struct SymbolTable *add_union_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
) {
  return add_symbol_core(
    symbol_string,
    primitive_type,
    structural_type,
    size,
    STORAGE_CLASS_UNION,
    0,
    &union_head,
    &union_tail,
    composite_type
  );
}

struct SymbolTable *add_enum_symbol(
  char *symbol_string,
  int storage_class,
  int value
) {
  return add_symbol_core(
    symbol_string,
    PRIMITIVE_INT,
    0,
    0,
    storage_class,
    value,
    &enum_head,
    &enum_tail,
    NULL
  );
}

struct SymbolTable *add_typedef_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int size,
  struct SymbolTable *composite_type
) {
  return add_symbol_core(
    symbol_string,
    primitive_type,
    structural_type,
    size,
    STORAGE_CLASS_TYPEDEF,
    0,
    &typedef_head,
    &typedef_tail,
    composite_type
  );
}

void clear_all_symbol_tables() {
  global_head = global_tail = NULL;
  local_head = local_tail = NULL;
  parameter_head = parameter_tail = NULL;
  composite_head = composite_tail = NULL;
  temp_member_head = temp_member_tail = NULL;
  struct_head = struct_tail = NULL;
  union_head = union_tail = NULL;
  enum_head = enum_tail = NULL;
  typedef_head = typedef_tail = NULL;
}

void clear_local_symbol_table() {
  local_head = local_tail = NULL;
  parameter_head = parameter_tail = NULL;
  current_function_symbol_id = NULL;
}
