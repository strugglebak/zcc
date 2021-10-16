#include <string.h>
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
static int parse_param_declaration_list(
  struct SymbolTable *old_function_symbol_table,
  struct SymbolTable *new_function_symbol_table
) {
  // 解析类似 void xxx(int a, int b) {} 函数中间的参数
  // 以及 struct xxx { int a; int b; }; 中间的成员变量
  int primitive_type = 0;
  //【定义】的函数的参数个数
  int parameter_count = 0;
  struct SymbolTable *prototype = NULL, *composite_type;

  if (old_function_symbol_table)
   prototype = old_function_symbol_table->member;

  // 开始解析参数
  while (token_from_file.token != TOKEN_RIGHT_PAREN) {
    primitive_type = parse_declaration_list(
      &composite_type,
      STORAGE_CLASS_FUNCTION_PARAMETER,
      TOKEN_COMMA,
      TOKEN_RIGHT_PAREN);

    if (primitive_type == -1)
      error("Bad type in parameter list");

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
    }

    // 解析完一轮参数/member，需要将个数记录下来
    parameter_count++;

    if (token_from_file.token == TOKEN_RIGHT_PAREN)
      break;

    verify_comma();
  }

  // 如果
  // 1. 已经【声明】过的函数/struct
  // 2. 解析完的参数个数跟已经【声明】过的函数/struct的参数/member个数不一致
  if (old_function_symbol_table && (parameter_count != old_function_symbol_table->element_number))
    error_with_message("Parameter count mismatch for function", old_function_symbol_table->name);

  return parameter_count;
}

static struct SymbolTable *parse_composite_declaration(int primitive_type) {
  // 解析 struct 定义的语句
  struct SymbolTable *composite_type = NULL, *member;
  int offset, member_primitive_type;

  // 跳过 struct/union 关键字
  scan(&token_from_file);

  // 判断 struct/union 后面的类型名字是否被定义过
  if (token_from_file.token == TOKEN_IDENTIFIER) {
    composite_type =
      primitive_type == PRIMITIVE_STRUCT
        ? find_struct_symbol(text_buffer)
        : find_union_symbol(text_buffer);
    // 跳过类型名字
    scan(&token_from_file);
  }

  // 如果下一个 token 不是 '{'，说明用户要用这个 struct/union 类型去声明变量
  // 那么直接返回即可
  if (token_from_file.token != TOKEN_LEFT_BRACE) {
    // 既然要声明变量，说明之前的 struct/union 声明的类型应该存在
    // 如果此时 composite_type 为空应该要报错
    if (!composite_type) error_with_message("Unknown struct/union type", text_buffer);
    return composite_type;
  }

  // 如果下一个 token 是 '{'，说明用户要用这个 struct/union 类型去做定义
  // 如果要做定义，那么此时 composite_type 应该要为空
  if (composite_type) error_with_message("Previously defined struct/union", text_buffer);

  // 开始构建 struct/union node
  composite_type =
    primitive_type == PRIMITIVE_STRUCT
      ? add_struct_symbol(
        text_buffer,
        PRIMITIVE_STRUCT,
        0,
        0,
        NULL)
      : add_union_symbol(
        text_buffer,
        PRIMITIVE_UNION,
        0,
        0,
        NULL);
  // 跳过 '{'
  scan(&token_from_file);

  // 在 member 列表中扫描
  while (1) {
    member_primitive_type
      = parse_declaration_list(&member, STORAGE_CLASS_MEMBER, TOKEN_SEMICOLON, TOKEN_RIGHT_BRACE);
    if (member_primitive_type == -1)
      error("Bad type in member list");
    if (token_from_file.token == TOKEN_SEMICOLON)
      scan(&token_from_file);
    if (token_from_file.token == TOKEN_RIGHT_BRACE)
      break;
  }

  verify_right_brace();

  if (!temp_member_head)
    error_with_message("No members in struct", composite_type->name);

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
    member->position =
      primitive_type == PRIMITIVE_STRUCT
        ? generate_align(member->primitive_type, offset, 1)
        // union 是共享变量地址，所以这里是 0
        : 0;
    offset += get_primitive_type_size(member->primitive_type, member->composite_type);
  }

  // 设置 struct 类型总共的大小
  composite_type->size = offset;
  return composite_type;
}

static void parse_enum_declaration() {
  // 解析类似于 enum xxx { a = 1, b } var1; 这样的语句
  struct SymbolTable *t = NULL;
  char *name;
  int value = 0;

  // 跳过 enum 关键字
  scan(&token_from_file);

  // 如果有已经声明的 xxx，就找出它
  if (token_from_file.token == TOKEN_IDENTIFIER) {
    t = find_enum_type_symbol(text_buffer);
    name = strdup(text_buffer);
    scan(&token_from_file);
  }

  // 如果下一个 token 不是 '{'，是类似这样的语句
  // enum xxx;
  if (token_from_file.token != TOKEN_LEFT_BRACE) {
    // 看 xxx 有没有被定义过
    if (!t) error_with_message("Undeclared enum type", name);
    return;
  }

  // 跳过 '{'
  scan(&token_from_file);

  // 如果已经存在 xxx，确保之前没有被定义过
  if (t) error_with_message("Enum type redeclared", t->name);

  // xxx 枚举类型加入 symbol table 链表
  t = add_enum_symbol(name, STORAGE_CLASS_ENUM_TYPE, 0);

  // 解析所有的 enum 枚举变量
  while (1) {
    // 拿到枚举变量名
    verify_identifier();
    name = strdup(text_buffer);

    // 确保枚举变量名不重复
    t = find_enum_value_symbol(name);
    if (t) error_with_message("Enum value redeclared", text_buffer);

    // 如果下一个 token 是 '='，比如 a = 1，把 '1' 取出来
    if (token_from_file.token == TOKEN_ASSIGN) {
      scan(&token_from_file);
      if (token_from_file.token != TOKEN_INTEGER_LITERAL)
        error("Expected int literal after '='");
      value = token_from_file.integer_value;
      scan(&token_from_file);
    }
    // 把 'a' 和 '1' 枚举变量加入 symbol table 链表
    t = add_enum_symbol(name, STORAGE_CLASS_ENUM_VALUE, value++);

    if (token_from_file.token == TOKEN_RIGHT_BRACE) break;
    verify_comma();
  }

  // 跳过 '}'
  scan(&token_from_file);
}

static int convert_multiply_token_2_primitive_type(int primitive_type) {
  while (token_from_file.token == TOKEN_MULTIPLY) {
    primitive_type = pointer_to(primitive_type);
    scan(&token_from_file);
  }
  return primitive_type;
}

static struct SymbolTable *parse_array_declaration(
  char *var_name,
  int primitive_type,
  struct SymbolTable *composite_type,
  int storage_class
) {
  struct SymbolTable *t;

  // 跳过 '['
  scan(&token_from_file);

  // 解析 '[0]' 中间的 0
  if (token_from_file.token == TOKEN_INTEGER_LITERAL) {
    switch (storage_class) {
      case STORAGE_CLASS_GLOBAL:
      case STORAGE_CLASS_EXTERN:
        t = add_global_symbol(
          var_name,
          pointer_to(primitive_type), // 变成指针类型
          STRUCTURAL_ARRAY,
          token_from_file.integer_value,
          storage_class,
          composite_type);
        break;
      case STORAGE_CLASS_LOCAL:
      case STORAGE_CLASS_FUNCTION_PARAMETER:
      case STORAGE_CLASS_MEMBER:
        error("For now, declaration of non-global arrays is not implemented");
    }
  }

  // 跳过 ']'
  scan(&token_from_file);
  verify_right_bracket();
  return t;
}

static struct SymbolTable *parse_scalar_declaration(
  char *var_name,
  int primitive_type,
  struct SymbolTable *composite_type,
  int storage_class
) {
  switch (storage_class) {
    case STORAGE_CLASS_GLOBAL:
    case STORAGE_CLASS_EXTERN:
      return add_global_symbol(
        var_name,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        storage_class,
        composite_type);
    case STORAGE_CLASS_LOCAL:
      return add_local_symbol(
        var_name,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
    case STORAGE_CLASS_FUNCTION_PARAMETER:
      return add_parameter_symbol(
        var_name,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
    case STORAGE_CLASS_MEMBER:
      return add_temp_member_symbol(
        var_name,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
  }
  return NULL;
}

static void parse_array_initialisation(
  struct SymbolTable *t,
  int primitive_type,
  struct SymbolTable *composite_type,
  int storage_class
) {
  error("No array initialisation yet");
}



// 解析变量或者函数定义
static struct SymbolTable *parse_symbol_declaration(
  int primitive_type,
  struct SymbolTable *composite_type,
  int storage_class
) {
  struct SymbolTable *t = NULL;
  char *var_name = strdup(text_buffer);
  int structural_type = STRUCTURAL_VARIABLE;

  verify_identifier();

  // 如果是 '(' 说明是函数定义
  if (token_from_file.token == TOKEN_LEFT_PAREN)
    return parse_function_declaration(primitive_type, var_name, composite_type, storage_class);

  // 先判断是否被定义过
  switch(storage_class) {
    case STORAGE_CLASS_GLOBAL:
    case STORAGE_CLASS_EXTERN:
      if (find_global_symbol(var_name))
        error_with_message("Duplicate global/extern variable declaration", var_name);
    case STORAGE_CLASS_LOCAL:
    case STORAGE_CLASS_FUNCTION_PARAMETER:
      if (find_local_symbol(var_name))
        error_with_message("Duplicate local/(function parameter) variable declaration", var_name);
    case STORAGE_CLASS_MEMBER:
      if (find_temp_member_symbol(var_name))
        error_with_message("Duplicate struct/union variable declaration", var_name);
  }

  // 如果是 '[' 说明是数组定义
  if (token_from_file.token == TOKEN_LEFT_BRACKET) {
    t = parse_array_declaration(var_name, primitive_type, composite_type, storage_class);
    structural_type = STRUCTURAL_ARRAY;
  } else
    // 如果不是当作普通变量处理
    t = parse_scalar_declaration(var_name, primitive_type, composite_type, storage_class);

  // 如果是 '=' 说明要有赋值操作
  if (token_from_file.token == TOKEN_ASSIGN) {
    // 以下两种类型是不能被赋值的
    if (storage_class == STORAGE_CLASS_FUNCTION_PARAMETER)
      error_with_message("Initialisation of a function parameter not permitted", var_name);
    if (storage_class == STORAGE_CLASS_MEMBER)
      error_with_message("Initialisation of a struct/union member not permitted", var_name);

    scan(&token_from_file);

    // 如果是数组要初始化
    if (structural_type == STRUCTURAL_ARRAY)
      parse_array_initialisation(t, primitive_type, composite_type, storage_class);
    else
      error("Scalar variable initialisation not done yet");
  }

  return t;
}

// typedef_declaration: 'typedef' identifier existing_type
//                    | 'typedef' identifier existing_type variable_name
//                    ;
int parse_typedef_declaration(struct SymbolTable **composite_type) {
  // 解析类似于 typedef int xxx; 类似的语句
  int primitive_type, storage_class = 0;

  // 跳过 typedef 关键字
  scan(&token_from_file);

  // 解析 int
  primitive_type = convert_token_2_primitive_type(composite_type, &storage_class);

  // 解析 xxx
  // 如果重复定义就报错
  if (find_typedef_symbol(text_buffer))
    error_with_message("Redefinition of typedef", text_buffer);

  // 如果没有重复定义就加入 typedef symbol table 链表
  add_typedef_symbol(text_buffer, primitive_type, 0, 0, *composite_type);

  // 跳过 ';'
  scan(&token_from_file);
  return primitive_type;
}

int parse_type_of_typedef_declaration(char *name, struct SymbolTable **composite_type) {
  // 解析类似的语句
  // typedef int FOO;
  // typedef FOO BAR;
  // BAR x;
  // x 就是 BAR 类型 ->FOO 类型 -> int 类型
  // 主要是解析 BAR x; 找出后面的 int
  struct SymbolTable *t = find_typedef_symbol(name);
  if (!t) error_with_message("Unknown type", name);

  scan(&token_from_file);
  *composite_type = t->composite_type;
  return t->primitive_type;
}

int convert_token_2_primitive_type(
  struct SymbolTable **composite_type,
  int *storage_class
) {
  int new_type, storage_class_change_to_extern = 1;

  // 先检查 storage class 是变成了 extern
  while (storage_class_change_to_extern) {
    switch (token_from_file.token) {
      case TOKEN_EXTERN:
        *storage_class = STORAGE_CLASS_EXTERN;
        scan(&token_from_file);
        break;
      default: storage_class_change_to_extern = 0;
    }
  }

  switch (token_from_file.token) {
    case TOKEN_CHAR: new_type = PRIMITIVE_CHAR; scan(&token_from_file); break;
    case TOKEN_INT: new_type = PRIMITIVE_INT; scan(&token_from_file); break;
    case TOKEN_VOID: new_type = PRIMITIVE_VOID; scan(&token_from_file); break;
    case TOKEN_LONG: new_type = PRIMITIVE_LONG; scan(&token_from_file); break;
    case TOKEN_STRUCT:
      new_type = PRIMITIVE_STRUCT;
      *composite_type = parse_composite_declaration(new_type);
      if (token_from_file.token == TOKEN_SEMICOLON)
        new_type = -1;
      break;
    case TOKEN_UNION:
      new_type = PRIMITIVE_UNION;
      *composite_type = parse_composite_declaration(new_type);
      if (token_from_file.token == TOKEN_SEMICOLON)
        new_type = -1;
      break;
    case TOKEN_ENUM:
      new_type = PRIMITIVE_INT;
      parse_enum_declaration();
      if (token_from_file.token == TOKEN_SEMICOLON)
        new_type = -1;
      break;
    case TOKEN_TYPEDEF:
      new_type = parse_typedef_declaration(composite_type);
      if (token_from_file.token == TOKEN_SEMICOLON)
        new_type = -1;
      break;
    case TOKEN_IDENTIFIER:
      // 在解析一个类型或者碰到关键字的时候，需要去查一下这个是不是被 typedef 定义过
      new_type = parse_type_of_typedef_declaration(text_buffer, composite_type);
      break;
    default:
      error_with_message("Illegal type, token", token_from_file.token_string);
  }

  return new_type;
}

struct SymbolTable *parse_function_declaration(
  int primitive_type,
  char *function_name,
  struct SymbolTable *composite_type,
  int storage_class
) {
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
      function_name,
      primitive_type,
      STRUCTURAL_FUNCTION,
      end_label,
      STORAGE_CLASS_GLOBAL,
      NULL);
  }

  // 开始解析函数参数
  verify_left_paren();
  parameter_count = parse_param_declaration_list(
    old_function_symbol_table,
    new_function_symbol_table);
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
  if (token_from_file.token == TOKEN_SEMICOLON)
    return old_function_symbol_table;

  // 到这一步说明是【定义】一个新函数
  current_function_symbol_id = old_function_symbol_table;

  // 如果是新函数那么应该初始化 loop level
  loop_level = 0;
  switch_level = 0;
  verify_left_brace();
  tree = parse_compound_statement(0);
  verify_right_brace();

  // 确保非 void 返回的函数始终有返回一个值
  if (primitive_type != PRIMITIVE_VOID) {
    if (!tree) error("No statements in function with non-void type");
    final_statement = tree->operation == AST_GLUE ? tree->right : tree;
    if (!final_statement ||
      final_statement->operation != AST_RETURN)
      error("No return for function with non-void type");
  }

  tree = create_ast_left_node(
    AST_FUNCTION,
    primitive_type,
    tree,
    end_label,
    old_function_symbol_table);

  if (output_dump_ast) {
    dump_ast(tree, NO_LABEL, 0);
    fprintf(stdout, "\n\n");
  }

  interpret_ast_with_register(tree, NO_LABEL, NO_LABEL, NO_LABEL, 0);

  clear_local_symbol_table();
  return old_function_symbol_table;
}

// 解析有初始化变量的地方
// 返回这些变量的 type
// 主要是解析类似于
// int xxx, yyy;
// 这种连续初始化的语句
int parse_declaration_list(
  struct SymbolTable **composite_type,
  int storage_class,
  int end_token,
  int end_token_2
) {
  int init_primitive_type, primitive_type;
  struct SymbolTable *t;

  // 检查是否是复合类型
  if ((init_primitive_type
    = convert_token_2_primitive_type(
      composite_type, &storage_class)) == -1)
    return init_primitive_type;

  // 解析定义列表
  while(1) {
    // 检查 symbol 是否为指针
    primitive_type = convert_multiply_token_2_primitive_type(init_primitive_type);

    // 解析 symbol
    t = parse_symbol_declaration(primitive_type, *composite_type, storage_class);

    // 函数不在初始化变量的范围，直接返回
    if (t->structural_type == STRUCTURAL_FUNCTION) {
      if (storage_class != STORAGE_CLASS_GLOBAL)
        error("Function definition not at global level");
      return primitive_type;
    }

    if (token_from_file.token == end_token ||
        token_from_file.token == end_token_2)
      return primitive_type;

    verify_comma();
  }
}

void parse_global_declaration() {
  struct SymbolTable *composite_type;

  // 解析声明的全局变量/函数
  while (token_from_file.token != TOKEN_EOF) {
    parse_declaration_list(
      &composite_type,
      STORAGE_CLASS_GLOBAL,
      TOKEN_SEMICOLON,
      TOKEN_EOF);

    if (token_from_file.token == TOKEN_SEMICOLON)
      scan(&token_from_file);
  }
}
