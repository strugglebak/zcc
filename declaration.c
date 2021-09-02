#include "definations.h"
#include "data.h"
#include "helper.h"
#include "scan.h"
#include "symbol_table.h"
#include "generator.h"
#include "ast.h"
#include "statement.h"
#include "types.h"

int convert_token_2_primitive_type() {
  int new_type;
  switch (token_from_file.token) {
    case TOKEN_CHAR: new_type = PRIMITIVE_CHAR; break;
    case TOKEN_INT: new_type = PRIMITIVE_INT; break;
    case TOKEN_VOID: new_type = PRIMITIVE_VOID; break;
    case TOKEN_LONG: new_type = PRIMITIVE_LONG; break;
    default:
      error_with_digital("Illegal type, token", token_from_file.token);
  }

  // 检查后面是否带 *
  while (1) {
    scan(&token_from_file);
    if (token_from_file.token != TOKEN_MULTIPLY) break;
    new_type = pointer_to(new_type);
  }
  return new_type;
}

void parse_var_declaration_statement(int primitive_type) {
  int symbol_table_index;

  // 解析类似于 int x,y,z; 这样的声明
  while (1) {
    // 把这个标识符加入 global symbol table
    symbol_table_index = add_global_symbol(text_buffer, primitive_type, STRUCTURAL_VARIABLE, 0);

    // 并接着生成对应的汇编代码
    generate_global_symbol_table_code(symbol_table_index);

    if (token_from_file.token == TOKEN_SEMICOLON) {
      scan(&token_from_file);
      return;
    }

    if (token_from_file.token == TOKEN_COMMA) {
      scan(&token_from_file);
      verify_identifier();
      continue;
    }
    error("Missing , or ; after identifier");
  }
}

struct ASTNode *parse_function_declaration_statement(int primitive_type) {
  struct ASTNode *tree, *final_statement;
  int name_slot, end_label;

  // 解析类似于 int xxx() {}; 这样的语句
  primitive_type = convert_token_2_primitive_type();
  verify_identifier();

  end_label = generate_label();
  name_slot = add_global_symbol(text_buffer, primitive_type, STRUCTURAL_FUNCTION, end_label);
  current_function_symbol_id = name_slot;

  verify_left_paren();
  verify_right_paren();

  tree = parse_compound_statement();

  // 确保非 void 返回的函数始终有返回一个值
  if (primitive_type != PRIMITIVE_VOID) {
    final_statement = tree->operation == AST_GLUE ? tree->right : tree;
    if (!final_statement ||
      final_statement->operation != AST_RETURN)
      error("No return for function with non-void type");
  }

  return create_ast_left_node(AST_FUNCTION, primitive_type, tree, name_slot);
}

void parse_global_declaration_statement() {
  struct ASTNode *tree;
  int primitive_type;

  // 解析声明的全局变量/函数
  while (token_from_file.token != TOKEN_EOF) {
    primitive_type = convert_token_2_primitive_type();
    verify_identifier();
    // 解析到 ( 说明是个函数
    if (token_from_file.token == TOKEN_LEFT_PAREN)
      tree = parse_function_declaration_statement(primitive_type);
    else
      // 否则就是变量
      parse_var_declaration_statement(primitive_type);
  }
}
