#include "parser.h"
#include "definations.h"
#include "generator.h"
#include "helper.h"
#include "data.h"
#include "helper.h"
#include "symbol_table.h"
#include "ast.h"

void parse_print_statement() {
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

void parse_assignment_statement() {
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
  tree = create_ast_node(AST_ASSIGNMENT_STATEMENT, left, right, 0);

  // 生成对应汇编代码
  interpret_ast_with_register(tree, -1);
  generate_clearable_registers();

  // 最后解析是否带分号
  verify_semicolon();
}

/**
 * 语句(statement) 的 BNF 为
 * statements: statement
 *      |      statement statements
 *      ;
 *
 * statement: 'print' expression ';'
 *      |     'int'   identifier ';'
 *      |     identifier '=' expression ';'
 *      ;
 *
 * identifier: TOKEN_IDENTIFIER
 *      ;
*/
void parse_statements() {
  while (1) {
    switch(token_from_file.token) {
      case TOKEN_PRINT:
        parse_print_statement();
        break;
      case TOKEN_INT:
        parse_var_declaration_statement();
        break;
      case TOKEN_IDENTIFIER:
        parse_assignment_statement();
        break;
      case TOKEN_EOF:
        return;
      default:
        error_with_digital("Syntax error, token", token_from_file.token);
    }
  }
}
