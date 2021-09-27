#include "definations.h"
#include "data.h"
#include "helper.h"
#include "scan.h"
#include "symbol_table.h"
#include "generator.h"
#include "ast.h"
#include "statement.h"
#include "types.h"
#include "declaration.h"
#include "parser.h"

// param_declaration: <null>
//           | variable_declaration
//           | variable_declaration ',' param_declaration
static int parse_parameter_declaration(int symbol_table_index) {
  // 解析类似 void xxx(int a, int b) {} 函数中间的参数
  int primitive_type = 0,
      parameter_symbol_table_index = symbol_table_index + 1;
  int old_parameter_count = 0;
  int parameter_count = 0;

  if (parameter_symbol_table_index)
    old_parameter_count = symbol_table[symbol_table_index].element_number;

  while (token_from_file.token != TOKEN_RIGHT_PAREN) {
    primitive_type = convert_token_2_primitive_type();
    verify_identifier();

    if (parameter_symbol_table_index) {
      if (primitive_type
        != symbol_table[symbol_table_index].primitive_type)
        error_with_digital(
          "Type doesn't match prototype for parameter",
          parameter_count + 1);
      parameter_symbol_table_index++;
    } else {
      // 函数参数中的这些定义属于 局部变量，同时也属于 参数定义
      parse_var_declaration_statement(primitive_type, STORAGE_CLASS_FUNCTION_PARAMETER);
    }
    parameter_count++;

    // 检查 , 或者 )，因为下一个 token 必然是 , 或者 )
    switch (token_from_file.token) {
      case TOKEN_COMMA:
        scan(&token_from_file);
        break;
      case TOKEN_RIGHT_PAREN: break;
      default:
        error_with_digital("Unexpected token in parameter list", token_from_file.token);
    }
  }

  if ((symbol_table_index != -1) &&
      (parameter_count != old_parameter_count))
    error_with_message("Parameter count mismatch for function",
      symbol_table[symbol_table_index].name);

  return parameter_count;
}

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

//  variable_declaration: type identifier ';'
//                      | type identifier '[' TOKEN_INTEGER_LITERAL ']' ';'
//                      ;
void parse_var_declaration_statement(int primitive_type, int storage_class) {
  int index;
  // 解析数组变量
  // 如果是 [
  if (token_from_file.token == TOKEN_LEFT_BRACKET) {
    // 跳过 [
    scan(&token_from_file);

    if (token_from_file.token == TOKEN_INTEGER_LITERAL) {
      // 目前来说不支持解析局部数组变量
      if (storage_class == STORAGE_CLASS_LOCAL)
        error("For now, declaration of local arrays is not implemented");

      add_global_symbol(
        text_buffer,
        pointer_to(primitive_type), // 变成指针类型
        STRUCTURAL_ARRAY,
        0,
        token_from_file.integer_value,
        storage_class);
    }

    // 检查 ]
    scan(&token_from_file);
    verify_right_bracket();
    return;
  }

  if (storage_class == STORAGE_CLASS_LOCAL) {
    index = add_local_symbol(
      text_buffer,
      primitive_type,
      STRUCTURAL_VARIABLE,
      1,
      storage_class);
    if (index < 0)
      error_with_message("Duplicate local variable declaration", text_buffer);
    return;
  }

  add_global_symbol(
    text_buffer,
    primitive_type,
    STRUCTURAL_VARIABLE,
    0,
    1,
    storage_class);
}

struct ASTNode *parse_function_declaration_statement(int primitive_type) {
  struct ASTNode *tree, *final_statement;
  int name_slot, end_label;
  int parameter_count;
  int symbol_table_index;

  if (symbol_table_index = find_symbol(text_buffer) != -1)
    if (symbol_table[symbol_table_index].structural_type != STRUCTURAL_FUNCTION)
      symbol_table_index = -1;

  if (symbol_table_index == -1) {
    end_label = generate_label();
    name_slot = add_global_symbol(
      text_buffer,
      primitive_type,
      STRUCTURAL_FUNCTION,
      end_label,
      0,
      STORAGE_CLASS_GLOBAL);
  }

  // 开始扫描函数参数
  verify_left_paren();
  parameter_count = parse_parameter_declaration(symbol_table_index);
  verify_right_paren();

  if (symbol_table_index == -1)
    symbol_table[name_slot].element_number = parameter_count;

  if (token_from_file.token == TOKEN_SEMICOLON) {
    scan(&token_from_file);
    return NULL;
  }

  if (symbol_table_index == -1)
    symbol_table_index = name_slot;
  copy_function_parameter(symbol_table_index);

  current_function_symbol_id = name_slot;

  tree = parse_compound_statement();

  // 确保非 void 返回的函数始终有返回一个值
  if (primitive_type != PRIMITIVE_VOID) {
    if (!tree) error("No statements in function with non-void type");
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
    if (token_from_file.token == TOKEN_LEFT_PAREN) {
      tree = parse_function_declaration_statement(primitive_type);
      if (output_dump_ast) {
        dump_ast(tree, NO_LABEL, 0);
        fprintf(stdout, "\n\n");
      }
      interpret_ast_with_register(tree, NO_LABEL, 0);
      reset_local_symbol_index();
    } else {
      // 否则就是变量
      parse_var_declaration_statement(primitive_type, 0, 0);
      verify_semicolon();
    }
  }
}
