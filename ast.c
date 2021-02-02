#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "definations.h"

struct ASTNode *create_ast_node(
  int operation,
  struct ASTNode *left,
  struct ASTNode *right,
  int interger_value
) {
  struct ASTNode *node;

  node = (struct ASTNode *)malloc(sizeof(struct ASTNode));
  if (NULL == node) {
    fprintf(stderr, "Unable to malloc in function make_ast_node\n");
    exit(1);
  }

  node->operation = operation;
  node->left = left;
  node->right = right;
  node->interger_value = interger_value;

  return node;
}

// 创建一个子节点
struct ASTNode *create_ast_leaf(int operation, int interger_value) {
  return create_ast_node(operation, NULL, NULL, interger_value);
}

struct ASTNode *create_ast_left_node(
  int operation,
  struct ASTNode *left,
  int interger_value
) {
  return create_ast_node(operation, left, NULL, interger_value);
}