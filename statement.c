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
  return create_ast_leaf(AST_BREAK, 0, 0, NULL);
}
static struct ASTNode *parse_continue_statement() {
  // 解析类似 continue; 这样的语句
  if (loop_level == 0)
    error("no loop to continue out from");
  scan(&token_from_file);
  return create_ast_leaf(AST_CONTINUE, 0, 0, NULL);
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
  verify_right_brace();

  // 检查 switch 里面语句的结果是一个 int 类型
  if (!check_int_type(left->primitive_type))
    error("Switch expression is not of integer type");

  // 创建一个 AST_SWITCH 的 node
  node = create_ast_left_node(AST_SWITCH, 0, left, 0, NULL);

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
          case_value = left->integer_value;

          // 遍历所有的 case value 列表，检查是否有重复的 case value
          // 比如 case a: 后面又有一个 case a:
          for (c = case_tree; c; c = c->right)
            if (case_value == c->integer_value)
              error("Duplicate case value");
        }

        // 跳过 ':'
        verify_colon();
        // 解析 case a: 里面的复合语句
        left = parse_compound_statement();
        // 同时计算有多少个 case
        case_count++;

        // 创建 case tree
        if (!case_tree) {
          case_tree = case_tail = create_ast_left_node(ast_operation, 0, left, case_value, NULL);
        } else {
          case_tail->right = create_ast_left_node(ast_operation, 0, left, case_value, NULL);
          case_tail = case_tail->right;
        }
        break;
      default:
        error_with_digital("Unexpected token in switch", token_from_file.token);
    }
  }
  switch_level--;

  // 解析 case 完之后给初始 node 赋值
  node->integer_value = case_count;
  node->right = case_tree;

  // 跳过 '}'
  verify_right_brace();

  return node;
}

static struct ASTNode *parse_single_statement() {
  int primitive_type, storage_class = STORAGE_CLASS_LOCAL;
  struct SymbolTable *composite_type;

  switch(token_from_file.token) {
    case TOKEN_IDENTIFIER:
      // 检查是否被 typedef 定义过
      if (!find_typedef_symbol(text_buffer))
        return converse_token_2_ast(0);
    case TOKEN_CHAR:
    case TOKEN_INT:
    case TOKEN_LONG:
    case TOKEN_STRUCT:
    case TOKEN_UNION:
    case TOKEN_ENUM:
    case TOKEN_TYPEDEF:
      primitive_type = convert_token_2_primitive_type(&composite_type, &storage_class);
      verify_identifier();
      parse_var_declaration_statement(primitive_type, storage_class, composite_type);
      verify_semicolon();
      return NULL;
    case TOKEN_IF:
      return parse_if_statement();
    case TOKEN_WHILE:
      return parse_while_statement();
    case TOKEN_FOR:
      return parse_for_statement();
    case TOKEN_RETURN:
      return parse_return_statement();
    case TOKEN_SWITCH:
      return parse_switch_statement();
    case TOKEN_BREAK:
      return parse_break_statement();
    case TOKEN_CONTINUE:
      return parse_continue_statement();
    default:
      return converse_token_2_ast(0);
  }
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
        NULL);
  verify_right_paren();

  // 为复合语句创建 ast
  true_node = parse_compound_statement();

  // 如果解析到下一步发现有 else，直接跳过，并同时为复合语句创建 ast
  if (token_from_file.token == TOKEN_ELSE) {
    scan(&token_from_file);
    false_node = parse_compound_statement();
  }

  return create_ast_node(
    AST_IF,
    PRIMITIVE_NONE,
    condition_node,
    true_node,
    false_node,
    0,
    NULL);
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
        NULL);

  verify_right_paren();

  // while 里面都是复合语句，所以直接解析即可

  loop_level++;
  statement_node = parse_compound_statement();
  loop_level--;
  return create_ast_node(
    AST_WHILE,
    PRIMITIVE_NONE,
    condition_node,
    NULL,
    statement_node,
    0,
    NULL);
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
    condition_node->operation > AST_COMPARE_GREATER_EQUALS)
    // 说明是一个条件语句或者是一个 int 数字之类的，将其 bool 化
    condition_node = create_ast_left_node(
        AST_TO_BE_BOOLEAN,
        condition_node->primitive_type,
        condition_node,
        0,
        NULL);
  verify_semicolon();

  // 解析 i = i+1)
  post_operation_statement_node = parse_single_statement();
  verify_right_paren();

  // 解析 for 语句块里面的 stmt
  loop_level++;
  statement_node = parse_compound_statement();
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
  tree = create_ast_node(AST_GLUE, PRIMITIVE_NONE, statement_node, NULL, post_operation_statement_node, 0, NULL);
  tree = create_ast_node(AST_WHILE, PRIMITIVE_NONE, condition_node, NULL, tree, 0, NULL);
  return create_ast_node(AST_GLUE, PRIMITIVE_NONE, pre_operation_statement_node, NULL, tree, 0, NULL);
}

static struct ASTNode *parse_return_statement() {
  struct ASTNode *tree;
  struct SymbolTable *t = current_function_symbol_id;

  // 如果函数返回的是 void 则不能进行返回
  if (t->primitive_type == PRIMITIVE_VOID)
    error("Can't return from a void function");

  // 检查 return (
  verify_token_and_fetch_next_token(TOKEN_RETURN, "return");
  verify_left_paren();

  // 解析 return 中间的语句
  tree = converse_token_2_ast(0);

  // 检查 return type 和 function type 是否兼容
  tree = modify_type(tree, t->primitive_type, 0);
  if (!tree) // 不允许强制转换
    error("Incompatible types to return");

  // 生成 return_statement 的 node
  tree = create_ast_left_node(AST_RETURN, PRIMITIVE_NONE, tree, 0, NULL);

  // 检查 )
  verify_right_paren();

  return tree;
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
        (tree->operation == AST_ASSIGN ||
         tree->operation == AST_RETURN ||
         tree->operation == AST_FUNCTION_CALL ||
         tree->operation == AST_BREAK ||
         tree->operation == AST_CONTINUE))
      verify_semicolon();

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
      left = left
        ? create_ast_node(AST_GLUE, PRIMITIVE_NONE, left, NULL, tree, 0, NULL)
        : tree;
    }

    // 最后解析右括号 }
    if (token_from_file.token == TOKEN_RIGHT_BRACE) {
      verify_right_brace();
      return left;
    }
  }
}
