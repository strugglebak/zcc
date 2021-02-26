#include "parser.h"
#include "definations.h"
#include "generator.h"
#include "helper.h"
#include "data.h"

/**
 * 语句(statement) 的 BNF 为
 * statements: statement
 *  | statement statements
 *  ;
 * statement: 'print' expression ';'
 *  ;
*/
void parse_statements() {
  struct ASTNode *tree;

  int register_index = 0;

  while (1) {
    verify_token_and_fetch_next_token(TOKEN_PRINT, "print");

    // 解析带 print 的 statement，并创建汇编代码
    tree = converse_token_2_ast(0);
    register_index = interpret_ast_with_register(tree);

    generate_printable_code(register_index);
    generate_clearable_registers();

    // 解析是否带了分号
    verify_semicolon();

    if (TOKEN_EOF == token_from_file.token) break;
  }
}
