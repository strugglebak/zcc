#include "statement.h"
#include "parser.h"
#include "definations.h"
#include "generator.h"
#include "helper.h"
#include "data.h"
#include "symbol_table.h"
#include "ast.h"
#include "scan.h"
#include "declaration.h"
#include "types.h"

static struct ASTNode *parse_return_statement();
static struct ASTNode *parse_break_statement() {
  // 解析类似 break; 这样的语句
  if (loop_level == 0 && switch_level == 0)
    error("no loop or switch to break out from");
  scan(&token_from_file);
  verify_semicolon();
  return (create_ast_leaf(AST_BREAK, PRIMITIVE_NONE, 0, NULL, NULL));
}
static struct ASTNode *parse_continue_statement() {
  // 解析类似 continue; 这样的语句
  if (loop_level == 0)
    error("no loop to continue out from");
  scan(&token_from_file);
  verify_semicolon();
  return (create_ast_leaf(AST_CONTINUE, PRIMITIVE_NONE, 0, NULL, NULL));
}

static struct ASTNode *parse_switch_statement() {
  struct ASTNode *left, *node, *c, *case_tree = NULL, *case_tail;
  int continue_loop = 1, case_count = 0, case_value, exist_default = 0, ast_operation;

  // 跳过 switch 以及 '('
  scan(&token_from_file);
  verify_left_paren();

  // 拿到 switch 里面的语句，并跳过 ')' 以及 '{'
  left = converse_token_2_ast(0);
  verify_right_paren();
  verify_left_brace();

  // 检查 switch 里面语句的结果是一个 int 类型
  if (!check_int_type(left->primitive_type))
    error("Switch expression is not of integer type");

  // 创建一个 AST_SWITCH 的 node
  node = create_ast_left_node(AST_SWITCH, 0, left, 0, NULL, NULL);

  // 开始解析 'case' 语句
  switch_level++;
  while (continue_loop) {
    switch(token_from_file.token) {
      // 如果是 switch(xxx) {} 这种情况，立刻退出
      case TOKEN_RIGHT_BRACE:
        if (case_count == 0)
          error("No cases in switch");
        continue_loop = 0;
        break;
      case TOKEN_CASE:
      case TOKEN_DEFAULT:
        // 先检查 'default' 之后是否还有 'case' 或者 'default'
        if (exist_default)
          error("case or default after existing default");

        if (token_from_file.token == TOKEN_DEFAULT) {
          ast_operation = AST_DEFAULT;
          exist_default = 1;
          scan(&token_from_file);
        } else {
          ast_operation = AST_CASE;
          scan(&token_from_file);

          // case a:
          left = converse_token_2_ast(0);

          // 检查 case a: 这个 a 是一个 int 类型的字面量
          if (left->operation != AST_INTEGER_LITERAL)
            error("Expecting integer literal for case value");
          case_value = left->ast_node_integer_value;

          // 遍历所有的 case value 列表，检查是否有重复的 case value
          // 比如 case a: 后面又有一个 case a:
          for (c = case_tree; c; c = c->right)
            if (case_value == c->ast_node_integer_value)
              error("Duplicate case value");
        }

        // 跳过 ':'
        verify_colon();
        // 同时计算有多少个 case
        case_count++;

        // 解析 case a: 里面的复合语句
        // 也支持 case a: case b: xxx(); break; 这样的语句
        if (token_from_file.token == TOKEN_CASE) {
          left = NULL;
        } else {
          left = parse_compound_statement(1);
        }

        // 创建 case tree
        if (!case_tree) {
          case_tree = case_tail = create_ast_left_node(ast_operation, 0, left, case_value, NULL, NULL);
        } else {
          case_tail->right = create_ast_left_node(ast_operation, 0, left, case_value, NULL, NULL);
          case_tail = case_tail->right;
        }
        break;
      default:
        error_with_message("Unexpected token in switch", token_from_file.token_string);
    }
  }
  switch_level--;

  // 解析 case 完之后给初始 node 赋值
  node->ast_node_integer_value = case_count;
  node->right = case_tree;

  // 跳过 '}'
  verify_right_brace();

  return (node);
}

static struct ASTNode *parse_single_statement() {
  struct SymbolTable *composite_type;
  struct ASTNode *statement;

  switch(token_from_file.token) {
    // ';' 就是一个空语句
    case TOKEN_SEMICOLON:
      verify_semicolon();
      break;
    case TOKEN_LEFT_BRACE:
      verify_left_brace();
      statement = parse_compound_statement(0);
      verify_right_brace();
      return (statement);
    case TOKEN_IDENTIFIER:
      // 检查是否被 typedef 定义过
      if (!find_typedef_symbol(text_buffer)) {
        statement = converse_token_2_ast(0);
        verify_semicolon();
        return (statement);
      }
    case TOKEN_CHAR:
    case TOKEN_INT:
    case TOKEN_LONG:
    case TOKEN_STRUCT:
    case TOKEN_UNION:
    case TOKEN_ENUM:
    case TOKEN_TYPEDEF:
      parse_declaration_list(
        &composite_type,
        STORAGE_CLASS_LOCAL,
        TOKEN_SEMICOLON,
        TOKEN_EOF,
        &statement);
      verify_semicolon();
      return (statement);
    case TOKEN_IF:
      return (parse_if_statement());
    case TOKEN_WHILE:
      return (parse_while_statement());
    case TOKEN_FOR:
      return (parse_for_statement());
    case TOKEN_RETURN:
      return (parse_return_statement());
    case TOKEN_SWITCH:
      return (parse_switch_statement());
    case TOKEN_BREAK:
      return (parse_break_statement());
    case TOKEN_CONTINUE:
      return (parse_continue_statement());
    default:
      statement = converse_token_2_ast(0);
      verify_semicolon();
      return (statement);
  }

  return (NULL);
}
struct ASTNode *parse_if_statement() {
  struct ASTNode *condition_node, *true_node, *false_node = NULL;

  // 解析 statement 中是否有 if(
  verify_if();
  verify_left_paren();

  condition_node = converse_token_2_ast(0);

  // 确保条件语句中出现的是正确的符号
  if (condition_node->operation < AST_COMPARE_EQUALS ||
    condition_node->operation > AST_COMPARE_GREATER_EQUALS)
    // 说明是一个条件语句或者是一个 int 数字之类的，将其 bool 化
    condition_node = create_ast_left_node(
        AST_TO_BE_BOOLEAN,
        condition_node->primitive_type,
        condition_node,
        0,
        NULL,
        condition_node->composite_type);
  verify_right_paren();

  // 为复合语句创建 ast
  true_node = parse_single_statement();

  // 如果解析到下一步发现有 else，直接跳过，并同时为复合语句创建 ast
  if (token_from_file.token == TOKEN_ELSE) {
    scan(&token_from_file);
    false_node = parse_single_statement();
  }

  return (
    create_ast_node(
      AST_IF,
      PRIMITIVE_NONE,
      condition_node,
      true_node,
      false_node,
      0,
      NULL,
      NULL)
  );
}

struct ASTNode *parse_while_statement() {
  struct ASTNode *condition_node, *statement_node = NULL;

  verify_while();
  verify_left_paren();

  // 检测 while 中的 条件语句
  condition_node = converse_token_2_ast(0);
  // 确保条件语句中出现的是正确的符号
  if (condition_node->operation < AST_COMPARE_EQUALS ||
    condition_node->operation > AST_COMPARE_GREATER_EQUALS)
    // 说明是一个条件语句或者是一个 int 数字之类的，将其 bool 化
    condition_node = create_ast_left_node(
        AST_TO_BE_BOOLEAN,
        condition_node->primitive_type,
        condition_node,
        0,
        NULL,
        condition_node->composite_type);

  verify_right_paren();

  // while 里面都是复合语句，所以直接解析即可

  loop_level++;
  statement_node = parse_single_statement();
  loop_level--;
  return (
    create_ast_node(
      AST_WHILE,
      PRIMITIVE_NONE,
      condition_node,
      NULL,
      statement_node,
      0,
      NULL,
      NULL)
  );
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
  pre_operation_statement_node = parse_expression_list(TOKEN_SEMICOLON);
  verify_semicolon();

  // 解析 i < 10;
  condition_node = converse_token_2_ast(0);
  // 确保条件语句中出现的是正确的符号
  if (condition_node->operation < AST_COMPARE_EQUALS ||
    condition_node->operation > AST_COMPARE_GREATER_EQUALS)
    // 说明是一个条件语句或者是一个 int 数字之类的，将其 bool 化
    condition_node = create_ast_left_node(
        AST_TO_BE_BOOLEAN,
        condition_node->primitive_type,
        condition_node,
        0,
        NULL,
        condition_node->composite_type);
  verify_semicolon();

  // 解析 i = i+1)
  post_operation_statement_node = parse_expression_list(TOKEN_RIGHT_PAREN);
  verify_right_paren();

  // 解析 for 语句块里面的 stmt
  loop_level++;
  statement_node = parse_single_statement();
  loop_level--;

  // 递归构建 for ast
  // for 语句的 ast 结构如下
  //        AST_GLUE
  //       /     \
  // preop           AST_WHILE
  //               /        \
  // true_or_false_condition  AST_GLUE
  //                          /    \
  //                 compound_stmt  postop
  tree = create_ast_node(AST_GLUE, PRIMITIVE_NONE, statement_node, NULL, post_operation_statement_node, 0, NULL, NULL);
  tree = create_ast_node(AST_WHILE, PRIMITIVE_NONE, condition_node, NULL, tree, 0, NULL, NULL);
  return (create_ast_node(AST_GLUE, PRIMITIVE_NONE, pre_operation_statement_node, NULL, tree, 0, NULL, NULL));
}

static struct ASTNode *parse_return_statement() {
  struct ASTNode *tree = NULL;
  int has_paren = 0;

  verify_return();

  if (token_from_file.token == TOKEN_LEFT_PAREN) {
    has_paren = 1;

    if (current_function_symbol_id->primitive_type == PRIMITIVE_VOID)
      error("Can't return from a void function");

    verify_left_paren();
    // 解析 return 中间的语句
    tree = converse_token_2_ast(0);
    // 检查 return type 和 function type 是否兼容
    tree = modify_type(tree, current_function_symbol_id->primitive_type, 0, current_function_symbol_id->composite_type);

    if (!tree) // 不允许强制转换
      error("Incompatible types to return");

    // 检查 )
    verify_right_paren();
  }

  // 生成 return_statement 的 node
  tree = create_ast_left_node(AST_RETURN, PRIMITIVE_NONE, tree, 0, NULL, NULL);

  if (!has_paren) {
    // 这里有两种情况
    // 看下一个 token 是不是 ;
    // 是就直接 verify semi
    // 目前仅支持 'return;' 以及 'return x;' 或者 'return 0;'
    // 这种情况
    if (token_from_file.token != TOKEN_SEMICOLON) {
      // 不是就 scan
      scan(&token_from_file);
    }
  }

  verify_semicolon();
  return (tree);
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
 *
 * assignment_statement: identifier '=' expression ';'
 *      ;
 *
 * if_statement: if_head
 *      |        if_head 'else' compound_statement
 *      ;
 *
 * while_statement: 'while' '(' true_or_false_expression ')' compound_statement
 *
 * for_statement: 'for' '('
 *  expression_list ';'
 *  true_or_false_expression ';'
 *  expression_list ')' compound_statement
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
 *
 * global_declarations: global_declarations
 *      |               global_declaration global_declarations
 *      ;
 *
 * global_declaration: function_declaration
 *      |              var_declaration
 *      ;
 *
 * function_declaration: type identifier '(' ')' compound_statement
 *      ;
 *
 * var_declaration: type identifier_list ';'
 *      ;
 *
 * type: type_keyword operation_pointer
 *      ;
 *
 * type_keyword: 'void' | 'char' | 'int' | 'long'
 *      ;
 *
 * operation_pointer: <empty> | '*' opt_pointer
 *      ;
 *
 * identifier_list: identifier
 *      |           identifier ',' identifier_list
 *      ;
 *
 * function_call: identifier '(' expression ')'
 *      ;
 *
 * return_statement: 'return' '(' expression ')'
 *      ;
 *
 * identifier: TOKEN_IDENTIFIER
 *      ;
*/
struct ASTNode *parse_compound_statement(int in_switch_statement) {
  struct ASTNode *left = NULL;
  struct ASTNode *tree;

  while (1) {
    // 可以允许 {} 里面的空语句
    if (token_from_file.token == TOKEN_RIGHT_BRACE) return (left);
    if (in_switch_statement && (
      token_from_file.token == TOKEN_CASE ||
      token_from_file.token == TOKEN_DEFAULT
    )) return (left);

    // 这里主要兼容对 for 语句的处理
    tree = parse_single_statement();

    // 如果 tree 不为空，则更新对应的 left
    // 变成如下的形式
    //          AST_GLUE
    //         /  \
    //     AST_GLUE stmt4
    //       /  \
    //   AST_GLUE stmt3
    //     /  \
    // stmt1  stmt2
    if (tree) {
      if (left != NULL) {
        left = create_ast_node(AST_GLUE, PRIMITIVE_NONE, left, NULL, tree, 0, NULL, NULL);
      } else {
        left = tree;
      }
    }
  }

  return (NULL);
}
