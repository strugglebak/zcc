#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "definations.h"
#include "data.h"
#include "ast.h"
#include "helper.h"

/**
 * 由于 converse_token_2_ast 得到的树结构对于要遍历它进行运算来说是没有优先级的，所以这里
 * 需要对这个构建好的树进行解释，然后得到一个准确的运算结果
 * 简单来说，就是要让
 *    *
 *   / \
 *  2   +
 *     / \
 *    3   *
 *       / \
 *      4   5
 * 变成
 *         +
 *        / \
 *       /   \
 *      /     \
 *     *       *
 *    / \     / \
 *   2   3   4   5
 *
 * 然后再对树进行后序遍历，并将其做运算，就能得到对应的结果
*/
int interpret_ast(struct ASTNode *node) {
  int left_value, right_value;

  if (node->left) {
    left_value = interpret_ast(node->left);
  }
  if (node->right) {
    right_value = interpret_ast(node->right);
  }

  switch (node->operation) {
  case AST_PLUS:
    return (left_value + right_value);
  case AST_MINUS:
    return (left_value - right_value);
  case AST_MULTIPLY:
    return (left_value * right_value);
  case AST_DIVIDE:
    return (left_value / right_value);
  case AST_INTEGER_LITERAL:
    return (node->ast_node_integer_value);
  default:
    error_with_digital("Unknown AST operator", node->operation);
  }

  return NO_REGISTER;
}
