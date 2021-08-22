#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "definations.h"
#include "data.h"
#include "ast.h"
#include "scan.h"
#include "helper.h"
#include "symbol_table.h"
#include "types.h"
#include "parser.h"

static int operation_precedence_array[] = {
  0, // TOKEN_EOF
  10, 10, // + -
  20, 20, // * /
  30, 30, // == !=
  40, 40, 40, 40 // < > <= >=
};

// 确定操作符的优先级
static int operation_precedence(int operation_in_token) {
  int precedence = operation_precedence_array[operation_in_token];
  if (!precedence) {
    fprintf(stderr, "Syntax error on line %d, token %d\n", line, operation_in_token);
    exit(1);
  }
  return precedence;
}

// 逐个字的读取文件，并将文件中读取到的数字字符构建成一颗树
static struct ASTNode *create_ast_node_from_expression() {
  struct ASTNode *node;
  int symbol_table_index;

  switch (token_from_file.token) {
    // 解析类似   char j; j= 2; 这样的语句需要考虑的情况
    // 即把 2 这个 int 数值转换成 char 类型的
    case TOKEN_INTEGER_LITERAL:
      node = create_ast_leaf(
        AST_INTEGER_LITERAL,
        token_from_file.integer_value >= 0 && token_from_file.integer_value < 256
          ? PRIMITIVE_CHAR
          : PRIMITIVE_INT,
        token_from_file.integer_value);
      break;
    case TOKEN_IDENTIFIER:
      // 解析类似的语句时需要注意的问题，即区别是变量名还是函数调用
      //  x = fred + jim;
      //  x = fred(5) + jim;

      // 扫描下一个，继续判断
      scan(&token_from_file);

      // 如果是左 (，则直接看作是函数调用
      if (token_from_file.token == TOKEN_LEFT_PAREN)
        return convert_function_call_2_ast();
      // 如果不是函数调用，则需要丢掉这个新的 token
      reject_token(&token_from_file);

      // 检查标识符是否存在
      if ((symbol_table_index = find_global_symbol_table_index(text_buffer)) == -1) {
        error_with_message("Unknown variable", text_buffer);
      }
      node = create_ast_leaf(
        AST_IDENTIFIER,
        global_symbol_table[symbol_table_index].primitive_type,
        symbol_table_index);
      break;
    default:
      error_with_digital("Syntax error on line", line);
  }

  // 扫描下一个，继续判断
  scan(&token_from_file);

  return node;
}

// 将 token 中的 + - * / 等转换成 ast 中的类型
int convert_token_operation_2_ast_operation(int operation_in_token) {
  if (operation_in_token > TOKEN_EOF && operation_in_token < TOKEN_INTEGER_LITERAL) {
    return operation_in_token;
  }
  error_with_digital("Syntax error, token", operation_in_token);
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
 * 注意这里用 pratt 解析的方式进行
*/
struct ASTNode *converse_token_2_ast(int previous_token_precedence) {
  struct ASTNode *left, *right;
  int node_operaion_type = 0;
  int left_primitive_type, right_primitive_type = 0;

  // 初始化左子树
  left = create_ast_node_from_expression();

  // 到这里说明已经遍历到文件尾，可以直接 return
  node_operaion_type = token_from_file.token;
  // 如果遇到分号，也可以直接 return
  // 如果遇到')' 说明这个节点已经构建完成，直接返回 left
  if (TOKEN_SEMICOLON == node_operaion_type ||
    TOKEN_EOF == node_operaion_type ||
    TOKEN_RIGHT_PAREN == node_operaion_type
    ) return left;

  // 如果这次扫描得到的操作符比之前的优先级高，则进行树的循环构建
  while(operation_precedence(node_operaion_type) > previous_token_precedence) {
    // 继续扫描
    scan(&token_from_file);
    // 开始构建右子树
    right = converse_token_2_ast(operation_precedence(node_operaion_type));

    // 检查 left 和 right 节点的 primitive_type 是否兼容
    left_primitive_type = left->primitive_type;
    right_primitive_type = right->primitive_type;
    if (!check_type_compatible(&left_primitive_type, &right_primitive_type, 0))
      error("Incompatible types");

    // 检查完之后，left_primitive_type 和 right_primitive_type 应该会有变化
    // 到这里基本是
    // int char
    // char int
    // int int
    // char char
    // 如果 left_primitive_type 或者 right_primitive_type 其中之一是 AST_WIDEN
    //
    if (left_primitive_type)
      left = create_ast_left_node(left_primitive_type, right->primitive_type, left, 0);
    if (right_primitive_type)
      right = create_ast_left_node(right_primitive_type, left->primitive_type, right, 0);

    // 将 Token 操作符类型转换成 AST 操作符类型
    node_operaion_type = convert_token_operation_2_ast_operation(node_operaion_type);
    // 开始构建左子树
    left = create_ast_node(node_operaion_type, left->primitive_type, left, NULL, right, 0);

    node_operaion_type = token_from_file.token;
    if (TOKEN_SEMICOLON == node_operaion_type ||
      TOKEN_EOF == node_operaion_type ||
      TOKEN_RIGHT_PAREN == node_operaion_type
      ) break;
  }

  // 返回这颗构建的树
  return left;
}

struct ASTNode *convert_function_call_2_ast() {
  struct ASTNode *tree;
  int symbol_table_index;

  // 解析类似于 xxx(1); 这样的函数调用

  // 检查是否未声明
  if ((symbol_table_index = find_global_symbol_table_index(text_buffer)) == -1)
    error_with_message("Undeclared function", text_buffer);

  // 解析左 (
  verify_left_paren();

  // 解析括号中的语句
  tree = converse_token_2_ast(0);

  // 保存函数返回的类型作为这个 node 的类型
  // 保存函数名在这个 symbol table 中的 index 索引
  tree = create_ast_left_node(
    AST_FUNCTION_CALL,
    global_symbol_table[symbol_table_index].primitive_type,
    tree,
    symbol_table_index);

  // 解析右 )
  verify_right_paren();

  return tree;
}
