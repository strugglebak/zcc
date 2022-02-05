#include "data.h"
#include "definations.h"
#include "declaration.h"
#include "ast.h"

static struct ASTNode *fold_2_children(struct ASTNode *node) {
  int value, left_value, right_value;

  left_value = node->left->ast_node_integer_value;
  right_value = node->right->ast_node_integer_value;

  switch (node->operation) {
    case AST_PLUS:
      value = left_value + right_value;
      break;
    case AST_MINUS:
      value = left_value - right_value;
      break;
    case AST_MULTIPLY:
      value = left_value * right_value;
      break;
    case AST_DIVIDE:
      if (!right_value) return (node);
      value = left_value / right_value;
      break;
    default:
      return (node);
  }

  return (create_ast_leaf(AST_INTEGER_LITERAL, node->primitive_type, value, NULL, NULL));
}

static struct ASTNode *fold_1_children(struct ASTNode *node) {
  int value = node->left->ast_node_integer_value;

  switch (node->operation) {
    // 如果是 x = 3000 + 1;
    // 这个 1 如果是 char 类型的，就需要 widen 至 int 类型
    case AST_WIDEN: break;
    case AST_INVERT: value = ~value; break;
    case AST_LOGIC_NOT: value = !value; break;
    default: return (node);
  }

  return (create_ast_leaf(AST_INTEGER_LITERAL, node->primitive_type, value, NULL, NULL));
}

static struct ASTNode *fold(struct ASTNode *node) {
  if (!node) return (NULL);

  node->left = fold(node->left);
  node->right = fold(node->right);

  if (node->left && node->left->operation == AST_INTEGER_LITERAL)
    if (node->right && node->right->operation == AST_INTEGER_LITERAL)
      node = fold_2_children(node);
    else
      node = fold_1_children(node);

  return (node);
}

struct ASTNode *optimise(struct ASTNode *node) {
  return (fold(node));
}
