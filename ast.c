#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "definations.h"
#include "helper.h"

struct ASTNode *create_ast_node(
  int operation,
  int primitive_type,
  struct ASTNode *left,
  struct ASTNode *middle,
  struct ASTNode *right,
  int integer_value,
  struct SymbolTable *symbol_table
) {
  struct ASTNode *node;

  node = (struct ASTNode *)malloc(sizeof(struct ASTNode));
  if (NULL == node) {
    error("Unable to malloc in function make_ast_node");
  }

  node->operation = operation;
  node->primitive_type = primitive_type;
  node->left = left;
  node->middle = middle;
  node->right = right;
  node->ast_node_integer_value = integer_value;
  node->symbol_table = symbol_table;

  return node;
}

// 创建一个子节点
struct ASTNode *create_ast_leaf(
  int operation,
  int primitive_type,
  int integer_value,
  struct SymbolTable *symbol_table
) {
  return create_ast_node(
    operation,
    primitive_type,
    NULL, NULL, NULL,
    integer_value,
    symbol_table
  );
}

struct ASTNode *create_ast_left_node(
  int operation,
  int primitive_type,
  struct ASTNode *left,
  int integer_value,
  struct SymbolTable *symbol_table
) {
  return create_ast_node(
    operation,
    primitive_type,
    left, NULL, NULL,
    integer_value,
    symbol_table
  );
}
