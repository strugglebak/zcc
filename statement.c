#include "statement.h"
#include "parser.h"
#include "definations.h"
#include "generator.h"
#include "helper.h"
#include "data.h"
#include "helper.h"
#include "symbol_table.h"
#include "ast.h"
#include "scan.h"
#include "declaraion.h"

static struct ASTNode *parse_single_statement() {
  switch(token_from_file.token) {
    case TOKEN_PRINT:
      return parse_print_statement();
    case TOKEN_INT:
      parse_var_declaration_statement();
      return NULL;
    case TOKEN_IDENTIFIER:
      return parse_assignment_statement();
    case TOKEN_IF:
      return parse_if_statement();
    case TOKEN_WHILE:
      return parse_while_statement();
    case TOKEN_FOR:
      return parse_for_statement();
    default:
      error_with_digital("Syntax error, token", token_from_file.token);
  }
}

struct ASTNode *parse_print_statement() {
  struct ASTNode *tree;
  int register_index = 0;

  verify_token_and_fetch_next_token(TOKEN_PRINT, "print");

  // 解析带 print 的 statement，并创建汇编代码
  tree = converse_token_2_ast(0);
  tree = create_ast_left_node(AST_PRINT, tree, 0);

  return tree;
}

 struct ASTNode *parse_assignment_statement() {
  struct ASTNode *tree, *left, *right;
  int symbol_table_index;

  // 先解析是不是一个标识符
  verify_identifier();

  // 找下这个标识符有没有重复定义过
  // 如果没被定义就用了，抛出异常
  if ((symbol_table_index = find_global_symbol_table_index(text_buffer)) == -1) {
    error_with_message("Undeclared variable", text_buffer);
  }

  // 创建右节点
  // 这里将左值塞到右节点的目的是在将值保存到变量之前，优先计算 statement 的值
  // 注意我们这里的左节点的操作优先级高于右节点
  right = create_ast_leaf(AST_LVALUE_IDENTIFIER, symbol_table_index);

  // 解析 statement 中是否有 = 号
  verify_token_and_fetch_next_token(TOKEN_EQUALS, "=");

  // 解析 statement（也就是右值），并创建左节点
  left = converse_token_2_ast(0);

  // 根据左右节点创建一颗有关 statement 的 ast 树
  tree = create_ast_node(AST_ASSIGNMENT_STATEMENT, left, NULL, right, 0);

  return tree;
}

struct ASTNode *parse_if_statement() {
  struct ASTNode *condition_node, *true_node, *false_node = NULL;

  // 解析 statement 中是否有 if(
  verify_if();
  verify_left_paren();

  condition_node = converse_token_2_ast(0);

  // 确保条件语句中出现的是正确的符号
  if (condition_node->operation < AST_COMPARE_EQUALS ||
    condition_node->operation > AST_COMPARE_GREATER_EQUALS) {
    error("Bad comparison operator");
  }
  verify_right_paren();

  // 为复合语句创建 ast
  true_node = parse_compound_statement();

  // 如果解析到下一步发现有 else，直接跳过，并同时为复合语句创建 ast
  if (token_from_file.token == TOKEN_ELSE) {
    scan(&token_from_file);
    false_node = parse_compound_statement();
  }

  return create_ast_node(AST_IF, condition_node, true_node, false_node, 0);
}

struct ASTNode *parse_while_statement() {
  struct ASTNode *condition_node, *statement_node = NULL;

  verify_while();
  verify_left_paren();

  // 检测 while 中的 条件语句
  condition_node = converse_token_2_ast(0);
  // 确保条件语句中出现的是正确的符号
  if (condition_node->operation < AST_COMPARE_EQUALS ||
    condition_node->operation > AST_COMPARE_GREATER_EQUALS) {
    error("Bad comparison operator");
  }

  verify_right_paren();

  // while 里面都是复合语句，所以直接解析即可

  statement_node = parse_compound_statement();
  return create_ast_node(AST_WHILE, condition_node, NULL, statement_node, 0);
}

struct ASTNode *parse_for_statement() {
  struct ASTNode
    *condition_node, *statement_node,
    *pre_operation_statement_node, *post_operation_statement_node,
    *tree = NULL;

  // 解析类似  for (i=1 ; i < 10 ; i= i + 1) print('xxx'); 这样的语法

  // 解析 for(
  verify_for();
  verify_left_paren();

  // 解析 i=1;
  pre_operation_statement_node = parse_single_statement();
  verify_semicolon();

  // 解析 i < 10;
  condition_node = converse_token_2_ast(0);
  // 确保条件语句中出现的是正确的符号
  if (condition_node->operation < AST_COMPARE_EQUALS ||
    condition_node->operation > AST_COMPARE_GREATER_EQUALS) {
    error("Bad comparison operator");
  }
  verify_semicolon();

  // 解析 i = i+1)
  post_operation_statement_node = parse_single_statement();
  verify_right_paren();

  // 解析 for 语句块里面的 stmt
  statement_node = parse_compound_statement();

  // 递归构建 for ast
  // for 语句的 ast 结构如下
  //        A_GLUE
  //       /     \
  // preop           A_WHILE
  //               /        \
  // true_or_false_condition  A_GLUE
  //                          /    \
  //                 compound_stmt  postop
  tree = create_ast_node(AST_GLUE, statement_node, NULL, post_operation_statement_node, 0);
  tree = create_ast_node(AST_WHILE, condition_node, NULL, tree, 0);
  return create_ast_node(AST_GLUE, pre_operation_statement_node, NULL, tree, 0);
}


/**
 * 语句(statement) 的 BNF 为
 * compound_statement: '{' '}'
 *      |      '{' statement '}'
 *      |      '{' statement statements '}'
 *      ;
 *
 * statements: statement
 *      |      statement statements
 *      ;
 *
 * statement: print_statement
 *      |     declaration_statement
 *      |     assignment_statement
 *      |     if_statement
 *      ;
 *
 * print_statement: 'print' expression ';'
 *      ;
 * declaration_statement: 'int' identifier ';'
 *      ;
 * assignment_statement: identifier '=' expression ';'
 *      ;
 * if_statement: if_head
 *      |        if_head 'else' compound_statement
 *      ;
 *
 * while_statement: 'while' '(' true_or_false_expression ')' compound_statement
 *
 * for_statement: 'for' '('
 *  pre_operation_statement ';'
 *  true_or_false_expression ';'
 *  post_operation_statement ')' compound_statement
 *      ;
 *
 *  pre_operation_statement: statement
 *      ;
 *  post_operation_statement: statement
 *      ;
 *
 * if_head: 'if' '(' true_or_false_expression ')' compound_statement
 *      ;
 *
 * function_declaration: 'void' identifier '(' ')' compound_statement
 *      ;
 *
 * identifier: TOKEN_IDENTIFIER
 *      ;
*/
struct ASTNode *parse_compound_statement() {
  struct ASTNode *left = NULL;
  struct ASTNode *tree;

  // 先解析左括号 {
  verify_left_brace();

  while (1) {
    // 这里主要兼容对 for 语句的处理
    tree = parse_single_statement();

    // 既然是解析 stmt，那么必须后面带 ;
    if (tree &&
      (tree->operation == AST_PRINT ||
      tree->operation == AST_ASSIGNMENT_STATEMENT))
      verify_semicolon();

    // 如果 tree 不为空，则更新对应的 left
    if (!tree) continue;
      // 变成如下的形式
      //          A_GLUE
      //         /  \
      //     A_GLUE stmt4
      //       /  \
      //   A_GLUE stmt3
      //     /  \
      // stmt1  stmt2
    left = left
      ? create_ast_node(AST_GLUE, left, NULL, tree, 0)
      : tree;

    // 最后解析右括号 }
    if (token_from_file.token == TOKEN_RIGHT_BRACE) {
      verify_right_brace();
      return left;
    }
  }
}
