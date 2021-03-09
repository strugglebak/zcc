#include "parser.h"
#include "definations.h"
#include "generator.h"
#include "helper.h"
#include "data.h"
#include "helper.h"
#include "symbol_table.h"

void parse_print_statement() {
  struct ASTNode *tree;
  int register_index = 0;

  verify_token_and_fetch_next_token(TOKEN_PRINT, "print");

  // 解析带 print 的 statement，并创建汇编代码
  tree = converse_token_2_ast(0);
  register_index = interpret_ast_with_register(tree);

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
 * identifier: T_IDENT
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
      case TOKEN_EQUALS:
        parse_assignment_statement();
        break;
      case TOKEN_EOF:
        break;
      default:
        error_with_digital("Syntax error, token", token_from_file.token);
    }
  }
}
