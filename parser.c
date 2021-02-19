#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "definations.h"
#include "data.h"
#include "ast.h"
#include "scan.h"

// 逐个字的读取文件，并将文件中读取到的数字字符构建成一颗树
static struct ASTNode *create_ast_node_from_expression() {
  struct ASTNode *node;

  switch (token_from_file.token) {
  case TOKEN_INTEGER_LITERAL:
    node = create_ast_leaf(AST_INTEGER_LITERAL, token_from_file.integer_value);
    // 扫描下一个，继续判断
    scan(&token_from_file);
    return node;
  default:
    fprintf(stderr, "Syntax error on line %d\n", line);
    exit(1);
  }
}

// 将 token 中的 + - * / 转换成 ast 中的类型
int convert_token_operation_2_ast_operation(int operation_in_token) {
  switch (operation_in_token) {
    case TOKEN_PLUS:
      return AST_PLUS;
    case TOKEN_MINUS:
      return AST_MINUS;
    case TOKEN_MULTIPLY:
      return AST_MULTIPLY;
    case TOKEN_DIVIDE:
      return AST_DIVIDE;
    default:
      fprintf(stderr, "Unknown token in function convert_token_operation_2_ast_operation on line %d\n", line);
      exit(1);
  }
}

/**
 * 这里就将文件中解析到的 token 转化成 ast
 * 注意这里相当于 后序遍历
 * 主要是能将类似于 2 * 3 + 4 * 5 之类的表达式转换成如下形式的树结构
 *    *
 *   / \
 *  2   +
 *     / \
 *    3   *
 *       / \
 *      4   5
 * 但是注意这里的树结构是没有优先级的
*/
struct ASTNode *converse_token_2_ast() {
  struct ASTNode *root, *left, *right;
  int node_operaion_type = 0;

  // 递归遍历左子树
  left = create_ast_node_from_expression();

  // 到这里说明已经遍历到文件尾，可以直接 return
  if (TOKEN_EOF == token_from_file.token) return left;

  // 将 Token 操作符类型转换成 AST 操作符类型
  node_operaion_type = convert_token_operation_2_ast_operation(token_from_file.token);

  // 继续扫描，注意这里 token_from_file 这个全局的变量中的 token 会变化
  scan(&token_from_file);

  // 递归遍历右子树
  right = converse_token_2_ast();

  // 递归遍历根节点
  root = create_ast_node(node_operaion_type, left, right, 0);

  return root;
}

/**
 * 这里就将文件中解析到的 token 转化成 ast
 * 注意这里相当于 后序遍历
 * 主要是能将类似于 2 * 3 + 4 * 5 之类的表达式转换成如下形式的树结构
 *         +
 *        / \
 *       /   \
 *      /     \
 *     *       *
 *    / \     / \
 *   2   3   4   5
 * 注意这里是有 「优先级」的，目前来说 * / 符号的优先级最高
 * 这里用递归下降法
*/
struct ASTNode *converse_token_2_multiplicative_ast() {
  struct ASTNode *left, *right;
  int node_operaion_type = 0;

  // 初始化左子树
  left = create_ast_node_from_expression();

  // 到这里说明已经遍历到文件尾，可以直接 return
  node_operaion_type = token_from_file.token;
  if (TOKEN_EOF == node_operaion_type) return left;

  // 如果遇到 * / 这种符号
  // 则循环构建 ast
  while(TOKEN_MULTIPLY == node_operaion_type || TOKEN_DIVIDE == node_operaion_type) {
    // 继续扫描
    scan(&token_from_file);
    // 开始构建右子树
    right = create_ast_node_from_expression();
    // 将 Token 操作符类型转换成 AST 操作符类型
    node_operaion_type = convert_token_operation_2_ast_operation(node_operaion_type);
    // 开始构建左子树
    left = create_ast_node(node_operaion_type, left, right, 0);

    node_operaion_type = token_from_file.token;
    if (TOKEN_EOF == node_operaion_type) break;
  }

  // 返回这颗构建的树
  return left;
}

/**
 * 如果输入是 2 + 4 + 6
 * 则最终构建出的 ast 为
 *      +
 *     / \
 *    +   6
 *   / \
 *  2   4
 *
*/
struct ASTNode *converse_token_2_additive_ast() {
  struct ASTNode *left, *right;
  int node_operaion_type = 0;

  // 初始化左子树
  left = converse_token_2_multiplicative_ast();

  // 到这里说明已经遍历到文件尾，可以直接 return
  node_operaion_type = token_from_file.token;
  if (TOKEN_EOF == node_operaion_type) return left;

  while(1) {
    // 继续扫描
    scan(&token_from_file);
    // 开始构建右子树
    right = converse_token_2_multiplicative_ast();
    // 将 Token 操作符类型转换成 AST 操作符类型
    node_operaion_type = convert_token_operation_2_ast_operation(node_operaion_type);
    // 开始构建左子树
    left = create_ast_node(node_operaion_type, left, right, 0);

    node_operaion_type = token_from_file.token;
    if (TOKEN_EOF == node_operaion_type) break;
  }

  // 返回这颗构建的树
  return left;
}
