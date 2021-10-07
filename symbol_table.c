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
    0,
    composite_type
  );
  append_to_symbol_table(head, tail, t);
  return t;
}

static struct SymbolTable *find_symbol_in_list(
  char *symbol_string,
  struct SymbolTable *list
) {
  for (; list; list = list->next) {
    if (list->name && (!strcmp(symbol_string, list->name)))
      return list;
  }
  return NULL;
}

struct SymbolTable *find_global_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, global_head);
}

struct SymbolTable *find_local_symbol(char *symbol_string) {
  struct SymbolTable *node;
  // 优先判断是否在函数体内，如果是则寻找 parameter
  if (current_function_symbol_id) {
    node = find_symbol_in_list(symbol_string, current_function_symbol_id->member);
    if (node) return node;
  }
  return find_symbol_in_list(symbol_string, local_head);
}

struct SymbolTable *find_composite_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, composite_head);
}

struct SymbolTable *find_temp_member_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, temp_member_head);
}

struct SymbolTable *find_struct_symbol(char *symbol_string) {
  return find_symbol_in_list(symbol_string, struct_head);
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
  // TODO: 这里可能有指针指向的 bug
  // printf("能打印出这里说明没问题 1\n");
  // printf("composite_type.name = %s\n", node->composite_type->name);
  // printf("能打印出这里说明没问题 2\n");
  node->next = NULL;
  node->member = NULL;

  if (storage_class == STORAGE_CLASS_GLOBAL)
    generate_global_symbol(node);

  return node;
}

/**
 * new 一个新 symbol table 并加入 global symbol table 中
*/
struct SymbolTable *add_global_symbol(
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
    STORAGE_CLASS_GLOBAL,
    &global_head,
    &global_tail,
    composite_type
  );
}

/**
 * new 一个新 symbol table 并加入 local symbol table 中
*/
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
    &local_head,
    &local_tail,
    composite_type
  );
}

/**
 * new 一个新 symbol table 并加入 parameter symbol table 中
*/
struct SymbolTable *add_parameter_symbol(
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
    STORAGE_CLASS_FUNCTION_PARAMETER,
    &parameter_head,
    &parameter_tail,
    composite_type
  );
}

/**
 * new 一个新 symbol table 并加入 temp_member symbol table 中
*/
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
    &temp_member_head,
    &temp_member_tail,
    composite_type
  );
}

/**
 * new 一个新 symbol table 并加入 struct symbol table 中
*/
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
    &struct_head,
    &struct_tail,
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
}

void clear_local_symbol_table() {
  local_head = local_tail = NULL;
  parameter_head = parameter_tail = NULL;
  current_function_symbol_id = NULL;
}
