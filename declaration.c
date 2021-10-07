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

/**
 * 如何解析函数声明和定义？
 * 比如
 * void xxx(int a, char b, long c);
 *
 * void xxx(int a, char b, long c) {...}
 *
 * 1. 解析 identifier 和 '('
 *
 * 2. 在 symbol table 中搜索这个 identifier
 *    2.1 如果这个 identifier 存在，说明之前的 identifier 已经被【声明】过
 *    2.2 拿到这个 identifier 在 symbole table 中的位置，以及它的参数个数
 *
 * 3. 循环地解析 parameters (也就是 int a, char b, long c...)
 *    3.1 如果之前的 identifier 已经被【声明】过，就用当前【定义】的 parameters 的类型和
 * 已经【声明】过的 parameters 的类型做比较。如果当前【定义】的 identifier 是一个完整的函数
 * ，就更新 symbol table 中对应的 parameters 的个数
 *    3.2 如果之前的 identifier 没有被【声明】过，直接把 parameters 加入 symbol table
 *
 * 4. 对于函数参数个数，确保【定义】的和【声明】的一致
 *
 * 5. 解析 ')'
 *    5.1 如果下一个 token 是 ';' 就结束
 *    5.2 如果下一个 token 是 '{'，循环地将 parameters 从 symbol table 的【全局区】复制到【
 * 局部区】，这样可以让它们在【局部区】的排列方式是反着的。
*/
// var_declaration_list: <null>
//           | variable_declaration
//           | variable_declaration separate_token var_declaration_list ;
static int parse_var_declaration_list(
  struct SymbolTable *t,
  int storage_class,
  int separate_token, // ',' 或者 ';'
  int end_token       // ')' 或者 '}'
) {
  // 解析类似 void xxx(int a, int b) {} 函数中间的参数
  // 以及 struct xxx { int a; int b; }; 中间的成员变量
  int primitive_type = 0;
  //【定义】的函数的参数个数
  int parameter_count = 0;
  struct SymbolTable *prototype = NULL, *composite_type;

  if (t) prototype = t->member;

  // 开始解析参数
  while (token_from_file.token != end_token) {
    primitive_type = convert_token_2_primitive_type(&composite_type);
    verify_identifier();

    // 如果已经有【声明】过的 函数/struct
    // 比较【定义】的函数和【声明】的 函数/struct 的参数的类型
    if (prototype) {
      if (primitive_type
        != prototype->primitive_type)
        // 比较出错在第几个参数/member
        error_with_digital(
          "Type doesn't match prototype for parameter",
          parameter_count + 1);
      // 要比较的参数/member可能不止一个
      prototype = prototype->next;
    } else {
      // 如果没有【声明】过的函数/struct
      // 把这些参数加入 symbol table
      // 函数参数中的这些定义属于 局部变量，同时也属于 参数/member 定义
      parse_var_declaration_statement(
        primitive_type,
        STORAGE_CLASS_FUNCTION_PARAMETER,
        composite_type);
    }

    // 解析完一轮参数/member，需要将个数记录下来
    parameter_count++;

    // 检查必须要有分割符',' 或者 ';' 或者结尾符号 ')' 或者 '}'
    if (token_from_file.token != separate_token &&
        token_from_file.token != end_token)
      error_with_digital("Unexpected token in parameter list", token_from_file.token);
    if (token_from_file.token == separate_token)
      scan(&token_from_file);
  }

  // 如果
  // 1. 已经【声明】过的函数/struct
  // 2. 解析完的参数个数跟已经【声明】过的函数/struct的参数/member个数不一致
  if (t && (parameter_count != t->element_number))
    error_with_message("Parameter count mismatch for function", t->name);

  return parameter_count;
}

static struct SymbolTable *parse_struct_declaration() {
  // 解析 struct 定义的语句
  struct SymbolTable *composite_type = NULL, *member;
  int offset;

  // 略过 struct 关键字
  scan(&token_from_file);

  // 判断 struct 后面的类型名字是否被定义过
  if (token_from_file.token == TOKEN_IDENTIFIER) {
    composite_type = find_struct_symbol(text_buffer);
    // 略过类型名字
    scan(&token_from_file);
  }

  // 如果下一个 token 不是 '{'，说明用户要用这个 struct 类型去声明变量
  // 那么直接返回即可
  if (token_from_file.token != TOKEN_LEFT_BRACE) {
    // 既然要声明变量，说明之前的 struct 声明的类型应该存在
    // 如果此时 composite_type 为空应该要报错
    if (!composite_type) error_with_message("Unknown struct type", text_buffer);
    return composite_type;
  }

  // 如果下一个 token 是 '{'，说明用户要用这个 struct 类型去做定义
  // 如果要做定义，那么此时 composite_type 应该要为空
  if (composite_type) error_with_message("Previously defined struct", text_buffer);

  // 开始构建 struct node
  composite_type = add_struct_symbol(
    text_buffer,
    PRIMITIVE_STRUCT,
    0,
    0,
    NULL);
  // 略过 '{'
  scan(&token_from_file);

  // 开始解析 struct 里面的成员
  parse_var_declaration_list(NULL, STORAGE_CLASS_MEMBER, TOKEN_SEMICOLON, TOKEN_RIGHT_BRACE);
  verify_right_brace();

  // 解析完成之后首指针指向 member
  composite_type->member = temp_member_head;
  temp_member_head = temp_member_tail = NULL;

  // 根据成员变量的类型设置 offset
  // 先设置首个成员变量
  member = composite_type->member;
  member->position = 0;
  offset = get_primitive_type_size(member->primitive_type, member->composite_type);

  // 再设置剩下的成员变量
  member = member->next;
  for (; member; member = member->next) {
    member->position = generate_align(member->primitive_type, offset, 1);
    offset += get_primitive_type_size(member->primitive_type, member->composite_type);
  }

  // 设置 struct 类型总共的大小
  composite_type->size = offset;
  return composite_type;
}

int convert_token_2_primitive_type(struct SymbolTable **composite_type) {
  int new_type;
  switch (token_from_file.token) {
    case TOKEN_CHAR: new_type = PRIMITIVE_CHAR; scan(&token_from_file); break;
    case TOKEN_INT: new_type = PRIMITIVE_INT; scan(&token_from_file); break;
    case TOKEN_VOID: new_type = PRIMITIVE_VOID; scan(&token_from_file); break;
    case TOKEN_LONG: new_type = PRIMITIVE_LONG; scan(&token_from_file); break;
    case TOKEN_STRUCT:
      new_type = PRIMITIVE_LONG;
      *composite_type = parse_struct_declaration();
      break;
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
struct SymbolTable *parse_var_declaration_statement(
  int primitive_type,
  int storage_class,
  struct SymbolTable *composite_type
) {
  struct SymbolTable *t = NULL;

  // 先检查变量是否已经被定义过
  switch (storage_class) {
    case STORAGE_CLASS_GLOBAL:
      if (find_global_symbol(text_buffer))
        error_with_message("Duplicate global variable declaration", text_buffer);
    case STORAGE_CLASS_LOCAL:
    case STORAGE_CLASS_FUNCTION_PARAMETER:
      if (find_local_symbol(text_buffer))
        error_with_message("Duplicate local variable declaration", text_buffer);
    case STORAGE_CLASS_MEMBER:
      if (find_temp_member_symbol(text_buffer))
        error_with_message("Duplicate struct/union member declaration", text_buffer);
  }

  // 解析数组变量
  // 如果是 [
  if (token_from_file.token == TOKEN_LEFT_BRACKET) {
    // 检查 [
    scan(&token_from_file);

    if (token_from_file.token == TOKEN_INTEGER_LITERAL) {
      switch (storage_class) {
        case STORAGE_CLASS_GLOBAL:
          t = add_global_symbol(
            text_buffer,
            pointer_to(primitive_type), // 变成指针类型
            STRUCTURAL_ARRAY,
            token_from_file.integer_value,
            composite_type);
          break;
        case STORAGE_CLASS_LOCAL:
        case STORAGE_CLASS_FUNCTION_PARAMETER:
        case STORAGE_CLASS_MEMBER:
          error("For now, declaration of non-global arrays is not implemented");
      }
    }

    // 检查 ]
    scan(&token_from_file);
    verify_right_bracket();
    return t;
  }

  // 解析普通变量
  switch (storage_class) {
    case STORAGE_CLASS_GLOBAL:
      t = add_global_symbol(
        text_buffer,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
      break;
    case STORAGE_CLASS_LOCAL:
      t = add_local_symbol(
        text_buffer,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
      break;
    case STORAGE_CLASS_FUNCTION_PARAMETER:
      t = add_parameter_symbol(
        text_buffer,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
      break;
    case STORAGE_CLASS_MEMBER:
      t = add_temp_member_symbol(
        text_buffer,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
      break;
  }

  return t;
}

struct ASTNode *parse_function_declaration_statement(int primitive_type) {
  struct ASTNode *tree, *final_statement;
  int end_label = 0, parameter_count = 0;
  struct SymbolTable *old_function_symbol_table = find_symbol(text_buffer),
                     *new_function_symbol_table = NULL;

  // 如果之前有相同的 identifier，但是这个 identifier 不是函数
  if (old_function_symbol_table)
    if (old_function_symbol_table->structural_type != STRUCTURAL_FUNCTION)
      old_function_symbol_table = NULL;

  // 满足上述情况，那么说明这个 identifier 是要被声明成函数的
  // 把它加入 symbol table
  if (!old_function_symbol_table) {
    end_label = generate_label();
    new_function_symbol_table = add_global_symbol(
      text_buffer,
      primitive_type,
      STRUCTURAL_FUNCTION,
      end_label,
      NULL);
  }

  // 开始解析函数参数
  verify_left_paren();
  parameter_count = parse_var_declaration_list(
    old_function_symbol_table,
    STORAGE_CLASS_FUNCTION_PARAMETER,
    TOKEN_COMMA,
    TOKEN_RIGHT_PAREN);
  verify_right_paren();

  // 新【声明】函数对应的位置需要记录函数参数的个数
  // 更新该函数第一个参数所在位置
  if (new_function_symbol_table) {
    new_function_symbol_table->element_number = parameter_count;
    new_function_symbol_table->member = parameter_head;
    old_function_symbol_table = new_function_symbol_table;
  }

  // 清除掉 parameter symbol table 头尾指针
  parameter_head = parameter_tail = NULL;

  // 如果此时的 token 是 ';'，说明只是【声明】函数而不是【定义】函数
  // 可以直接退出
  if (token_from_file.token == TOKEN_SEMICOLON) {
    scan(&token_from_file);
    return NULL;
  }

  // 到这一步说明是【定义】一个新函数
  current_function_symbol_id = old_function_symbol_table;

  tree = parse_compound_statement();

  // 确保非 void 返回的函数始终有返回一个值
  if (primitive_type != PRIMITIVE_VOID) {
    if (!tree) error("No statements in function with non-void type");
    final_statement = tree->operation == AST_GLUE ? tree->right : tree;
    if (!final_statement ||
      final_statement->operation != AST_RETURN)
      error("No return for function with non-void type");
  }

  return create_ast_left_node(
    AST_FUNCTION,
    primitive_type,
    tree,
    end_label,
    old_function_symbol_table);
}

void parse_global_declaration_statement() {
  struct ASTNode *tree;
  int primitive_type;
  struct SymbolTable *composite_type;

  // 解析声明的全局变量/函数
  while (token_from_file.token != TOKEN_EOF) {
    primitive_type = convert_token_2_primitive_type(&composite_type);

    // 如果碰到 struct xxx; 类似的语句，这里应该不支持
    if (primitive_type == PRIMITIVE_STRUCT &&
        token_from_file.token == TOKEN_SEMICOLON) {
      scan(&token_from_file);
      continue;
    }

    printf("11111111\n");
    verify_identifier();
    printf("222222222\n");
    // 解析到 ( 说明是个函数
    if (token_from_file.token == TOKEN_LEFT_PAREN) {
      tree = parse_function_declaration_statement(primitive_type);

      // 如果是【声明】函数，则不用生成汇编
      if (!tree) continue;

      if (output_dump_ast) {
        dump_ast(tree, NO_LABEL, 0);
        fprintf(stdout, "\n\n");
      }
      interpret_ast_with_register(tree, NO_LABEL, 0);
      clear_local_symbol_table();
    } else {
      // 否则就是变量
      parse_var_declaration_statement(primitive_type, STORAGE_CLASS_GLOBAL, composite_type);
      verify_semicolon();
    }
  }
}
