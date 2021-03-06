#include "parser.h"
#include "definations.h"
#include "generator.h"
#include "helper.h"
#include "data.h"
#include "helper.h"
#include "symbol_table.h"
#include "ast.h"

struct ASTNode *parse_print_statement() {
  struct ASTNode *tree;
  int register_index = 0;

  verify_token_and_fetch_next_token(TOKEN_PRINT, "print");

  // 解析带 print 的 statement，并创建汇编代码
  tree = converse_token_2_ast(0);
  register_index = interpret_ast_with_register(tree, -1);

  generate_printable_code(register_index);
  generate_clearable_registers();

  // 解析是否带了分号
  verify_semicolon();

  return tree;
}

void parse_var_declaration_statement() {
  // 解析 int 后面是不是跟着一个标识符
  verify_token_and_fetch_next_token(TOKEN_INT, "int");
  verify_identifier();

  // 把这个标识符加入 global symbol table
  add_global_symbol(text_buffer);

  // 并接着生成对应的汇编代码
  generate_global_symbol_table_code(text_buffer);

  // 解析是否带了分号
  verify_semicolon();
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

  // 生成对应汇编代码
  interpret_ast_with_register(tree, -1);
  generate_clearable_registers();

  // 最后解析是否带分号
  verify_semicolon();

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
 * declaration_statement: 'int' identifier ';'
 *      ;
 * assignment_statement: identifier '=' expression ';'
 *      ;
 * if_statement: if_head
 *      |        if_head 'else' compound_statement
 *      ;
 *
 * if_head: 'if' '(' true_or_false_expression ')' compound_statement
 *      ;
 *
 * identifier: TOKEN_IDENTIFIER
 *      ;
*/
void parse_compound_statement() {
  struct ASTNode *left = NULL;
  struct ASTNode *tree;

  // 先解析左 {
  parse_left_brace();

  while (1) {
    switch(token_from_file.token) {
      case TOKEN_PRINT:
        tree = parse_print_statement();
        break;
      case TOKEN_INT:
        parse_var_declaration_statement();
        tree = NULL;
        break;
      case TOKEN_IDENTIFIER:
        tree = parse_assignment_statement();
        break;
      case TOKEN_IF:
        tree = parse_if_statement();
        break;
      case TOKEN_RIGHT_BRACE:
        // 解析右 }
        parse_right_brace();
        return left;
      case TOKEN_EOF:
        return;
      default:
        error_with_digital("Syntax error, token", token_from_file.token);
    }

    // 如果 tree 不为空，则更新对应的 left
    if (!tree) continue;
    left = left
      ? create_ast_node(AST_GLUE, left, NULL, tree, 0)
      : tree;
  }
}
