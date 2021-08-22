#include "definations.h"
#include "data.h"
#include "helper.h"
#include "scan.h"
#include "symbol_table.h"
#include "generator.h"
#include "ast.h"
#include "statement.h"

int convert_token_2_primitive_type(int token) {
  switch (token) {
    case TOKEN_CHAR: return PRIMITIVE_CHAR;
    case TOKEN_INT: return PRIMITIVE_INT;
    case TOKEN_VOID: return PRIMITIVE_VOID;
    case TOKEN_LONG: return PRIMITIVE_LONG;
    default:
      error_with_digital("Illegal type, token", token);
      break;
  }
}

void parse_var_declaration_statement() {
  int identifier_id, primitive_type;

  // 解析类似于 int xxx; 这样的语句
  primitive_type = convert_token_2_primitive_type(token_from_file.token);
  scan(&token_from_file);
  verify_identifier();

  // 把这个标识符加入 global symbol table
  identifier_id = add_global_symbol(text_buffer, primitive_type, STRUCTURAL_VARIABLE, 0);

  // 并接着生成对应的汇编代码
  generate_global_symbol_table_code(identifier_id);

  // 解析是否带了分号
  verify_semicolon();
}

struct ASTNode *parse_function_declaration_statement() {
  struct ASTNode *tree, *final_statement;
  int name_slot, primitive_type, end_label;

  // 解析类似于 int xxx() {}; 这样的语句
  primitive_type = convert_token_2_primitive_type(token_from_file.token);
  scan(&token_from_file);
  verify_identifier();

  end_label = generate_label();
  name_slot = add_global_symbol(text_buffer, primitive_type, STRUCTURAL_FUNCTION, end_label);
  current_function_symbol_id = name_slot;
  printf("current func = %d\n %s\n", current_function_symbol_id, text_buffer);

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
