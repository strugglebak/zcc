#include <string.h>
#include <stdlib.h>
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
#include "optimizer.h"

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
  struct ASTNode *tree;

  if (old_function_symbol_table)
   prototype = old_function_symbol_table->member;

  // 开始解析参数
  while (token_from_file.token != TOKEN_RIGHT_PAREN) {
    // 如果第一个 token 是 'void'
    if (token_from_file.token == TOKEN_VOID) {
      // 用一个替代指针提前查看
      scan(&look_ahead_token);
      // 如果是 xxx(void) 这种
      if (look_ahead_token.token == TOKEN_RIGHT_PAREN) {
        parameter_count = 0;
        scan(&token_from_file);
        break;
      }
    }

    primitive_type = parse_declaration_list(
      &composite_type,
      STORAGE_CLASS_FUNCTION_PARAMETER,
      TOKEN_COMMA,
      TOKEN_RIGHT_PAREN,
      &tree);

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

  return (parameter_count);
}

static struct SymbolTable *parse_composite_declaration(int primitive_type) {
  // 解析 struct 定义的语句
  struct SymbolTable *composite_type = NULL, *member;
  int offset, member_primitive_type;
  struct ASTNode *tree;

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
    return (composite_type);
  }

  // 如果下一个 token 是 '{'，说明用户要用这个 struct/union 类型去做定义
  // 如果要做定义，那么此时 composite_type 应该要为空
  if (composite_type) error_with_message("Previously defined struct/union", text_buffer);

  // 开始构建 struct/union node
  composite_type =
    primitive_type == PRIMITIVE_STRUCT
      ? add_struct_symbol(text_buffer)
      : add_union_symbol(text_buffer);
  // 跳过 '{'
  scan(&token_from_file);

  // 在 member 列表中扫描
  while (1) {
    member_primitive_type
      = parse_declaration_list(
          &member,
          STORAGE_CLASS_MEMBER,
          TOKEN_SEMICOLON,
          TOKEN_RIGHT_BRACE,
          &tree);
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
  member->symbol_table_position = 0;
  offset = get_primitive_type_size(member->primitive_type, member->composite_type);

  // 再设置剩下的成员变量
  member = member->next;
  for (; member; member = member->next) {
    member->symbol_table_position =
      primitive_type == PRIMITIVE_STRUCT
        ? generate_align(member->primitive_type, offset, 1)
        // union 是共享变量地址，所以这里是 0
        : 0;
    offset += get_primitive_type_size(member->primitive_type, member->composite_type);
  }

  // 设置 struct 类型总共的大小
  composite_type->size = offset;
  return (composite_type);
}

static void parse_enum_declaration() {
  // 解析类似于 enum xxx { a = 1, b } var1; 这样的语句
  struct SymbolTable *t = NULL;
  char *name = NULL;
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

int convert_multiply_token_2_primitive_type(int primitive_type) {
  while (token_from_file.token == TOKEN_MULTIPLY) {
    primitive_type = pointer_to(primitive_type);
    scan(&token_from_file);
  }
  return (primitive_type);
}

int convert_literal_token_2_integer_value(int primitive_type) {
  struct ASTNode *tree = optimise(converse_token_2_ast(0));

  // 如果有类型强制转换，标记 tree child 有强制转换的类型
  // char *c = (char *)0;
  if (tree->operation == AST_TYPE_CASTING) {
    tree->left->primitive_type = tree->primitive_type;
    tree = tree->left;
  }

  if (tree->operation != AST_INTEGER_LITERAL &&
      tree->operation != AST_STRING_LITERAL)
    error("Cannot initialise globals with a general expression");

  // 如果是 char *
  if (primitive_type == pointer_to(PRIMITIVE_CHAR)) {
    if (tree->operation == AST_STRING_LITERAL)
      return (tree->ast_node_integer_value);
    if (tree->operation == AST_INTEGER_LITERAL &&
        !tree->ast_node_integer_value)
      return (0);
  }

  if (check_int_type(primitive_type) &&
      (get_primitive_type_size(primitive_type, NULL) >=
        get_primitive_type_size(tree->primitive_type, NULL)))
    return (tree->ast_node_integer_value);

  error("Type mismatch: literal vs. variable");
  return (0);
}

int convert_type_casting_token_2_primitive_type(struct SymbolTable **composite_type) {
  int primitive_type, storage_class;

  primitive_type = convert_multiply_token_2_primitive_type(
    convert_token_2_primitive_type(
      composite_type,
      &storage_class
    )
  );

  if (
    primitive_type == PRIMITIVE_STRUCT ||
    primitive_type == PRIMITIVE_UNION ||
    primitive_type == PRIMITIVE_VOID
  )
    error("Cannot cast to a struct, union or void type");

  return (primitive_type);
}

static struct SymbolTable *parse_array_declaration(
  char *var_name,
  int primitive_type,
  struct SymbolTable *composite_type,
  int storage_class
) {
  // 解析类似于
  // int a[10];
  // char a[] = {'1', '2'};
  // char a[2] = {'1', '2'}
  struct SymbolTable *t;
  int element_number = -1,
      max_element_number,
      *init_value_list,
      i = 0, j;

  // 跳过 '['
  scan(&token_from_file);

  // 获取数组的大小
  if (token_from_file.token != TOKEN_RIGHT_BRACKET) {
    element_number = convert_literal_token_2_integer_value(PRIMITIVE_INT);
    if (element_number <= 0)
      error_with_digital("Array size is illegal", element_number);
  }

  // 跳过 ']'
  verify_right_bracket();

  // 把 'a' 加入 symbol table
  switch (storage_class) {
    case STORAGE_CLASS_STATIC:
    case STORAGE_CLASS_GLOBAL:
    case STORAGE_CLASS_EXTERN:
      t = find_global_symbol(var_name);
      if (is_new_symbol(t, storage_class, pointer_to(primitive_type), composite_type))
        t = add_global_symbol(
          var_name,
          pointer_to(primitive_type), // 变成指针类型
          STRUCTURAL_ARRAY,
          0,
          storage_class,
          composite_type,
          0);
      break;
    case STORAGE_CLASS_LOCAL:
      t = add_local_symbol(var_name, pointer_to(primitive_type), STRUCTURAL_ARRAY, 0, composite_type);
      break;
    default:
      error("Declaration of array parameters is not implemented");
  }

  // 数组赋值
  if (token_from_file.token == TOKEN_ASSIGN) {
    if (storage_class != STORAGE_CLASS_GLOBAL &&
        storage_class != STORAGE_CLASS_STATIC)
      error_with_message("Variable can not be initialised", var_name);
    // 跳过 '='
    scan(&token_from_file);
    // 跳过 '{'
    verify_left_brace();

#define DEFAULT_TABLE_INCREMENT 10

    max_element_number = element_number != -1
      ? element_number
      : DEFAULT_TABLE_INCREMENT;

    init_value_list = (int *)malloc(max_element_number * sizeof(int));

    // 处理 '{}' 中初始化的内容
    while (1) {
      if (element_number != -1 && i == max_element_number)
        error("Too many values in initialisation list");

      init_value_list[i++] = convert_literal_token_2_integer_value(primitive_type);

      // 如果是类似这种语句
      // char a[] = {'1', '2'};
      if (element_number == -1 && i == max_element_number) {
        // 自动扩容
        max_element_number += DEFAULT_TABLE_INCREMENT;
        init_value_list = (int *)realloc(init_value_list, max_element_number * sizeof(int));
      }

      if (token_from_file.token == TOKEN_RIGHT_BRACE) {
        // 跳过 '}'
        scan(&token_from_file);
        break;
      }

      // 检查 ','
      verify_comma();
    }

    // 没用到的空间初始化为 0
    for (j = i; j < t->element_number; j++) init_value_list[j] = 0;
    if (i > element_number) element_number = i;
    t->init_value_list = init_value_list;
  }

  // 设置一个数组变量的 element number 和 size 之前，先检查下这个变量是否是 extern 声明的
  // 如果不是并且 element number <= 0
  if (storage_class != STORAGE_CLASS_EXTERN && element_number <= 0)
    error_with_message("Array must have non-zero elements", t->name);

  t->element_number = element_number;
  t->size = t->element_number * get_primitive_type_size(primitive_type, composite_type);

  if (storage_class == STORAGE_CLASS_GLOBAL ||
      storage_class == STORAGE_CLASS_STATIC)
    generate_global_symbol(t);

  return (t);
}

static struct SymbolTable *parse_scalar_declaration(
  char *var_name,
  int primitive_type,
  struct SymbolTable *composite_type,
  int storage_class,
  struct ASTNode **tree
) {
  struct SymbolTable *t = NULL;
  struct ASTNode *var_node, *expression_node;

  *tree = NULL;

  switch (storage_class) {
    case STORAGE_CLASS_STATIC:
    case STORAGE_CLASS_GLOBAL:
    case STORAGE_CLASS_EXTERN:
      t = find_global_symbol(var_name);
      if (is_new_symbol(t, storage_class, primitive_type, composite_type))
        t = add_global_symbol(
          var_name,
          primitive_type,
          STRUCTURAL_VARIABLE,
          1,
          storage_class,
          composite_type,
          0);
      break;
    case STORAGE_CLASS_LOCAL:
      t = add_local_symbol(
        var_name,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
      break;
    case STORAGE_CLASS_FUNCTION_PARAMETER:
      t = add_parameter_symbol(
        var_name,
        primitive_type,
        STRUCTURAL_VARIABLE,
        composite_type);
      break;
    case STORAGE_CLASS_MEMBER:
      t = add_temp_member_symbol(
        var_name,
        primitive_type,
        STRUCTURAL_VARIABLE,
        1,
        composite_type);
      break;
  }

  // 如果变量要执行赋值操作
  // 比如 int x = (long)5;
  if (token_from_file.token == TOKEN_ASSIGN) {
    if (storage_class != STORAGE_CLASS_GLOBAL &&
        storage_class != STORAGE_CLASS_STATIC &&
        storage_class != STORAGE_CLASS_LOCAL)
      error_with_message("Variable can not be initialised", var_name);
    // 跳过 '='
    scan(&token_from_file);

    // 全局变量必须被赋值为一个字面量的值
    if (storage_class == STORAGE_CLASS_GLOBAL ||
        storage_class == STORAGE_CLASS_STATIC) {
      t->init_value_list = (int *)malloc(sizeof(int));
      t->init_value_list[0] = convert_literal_token_2_integer_value(primitive_type);
    }

    if (storage_class == STORAGE_CLASS_LOCAL) {
      // hack: 在这个函数里面创建 ast node
      var_node = create_ast_leaf(AST_IDENTIFIER, t->primitive_type, 0, t, t->composite_type);

      // 表达式的值为右值
      expression_node = converse_token_2_ast(0);
      expression_node->rvalue = 1;

      // 检查类型
      expression_node = modify_type(expression_node, var_node->primitive_type, 0, var_node->composite_type);
      if (!expression_node)
        error("Incompatible expression in assignment");

      // 创建 local 变量相关 ast tree
      *tree = create_ast_node(
        AST_ASSIGN,
        expression_node->primitive_type,
        expression_node,
        NULL,
        var_node,
        0,
        NULL,
        expression_node->composite_type);
    }
  }

  // 全局变量应该要先生成对应的汇编代码
  if (storage_class == STORAGE_CLASS_GLOBAL ||
      storage_class == STORAGE_CLASS_STATIC)
    generate_global_symbol(t);

  return (t);
}

// 解析变量或者函数定义
static struct SymbolTable *parse_symbol_declaration(
  int primitive_type,
  struct SymbolTable *composite_type,
  int storage_class,
  struct ASTNode **tree
) {
  struct SymbolTable *t = NULL;
  char *var_name = strdup(text_buffer);

  verify_identifier();

  // 如果是 '(' 说明是函数定义
  if (token_from_file.token == TOKEN_LEFT_PAREN)
    return (
      parse_function_declaration(primitive_type, var_name, composite_type, storage_class)
    );

  // 先判断是否被定义过
  switch(storage_class) {
    case STORAGE_CLASS_STATIC:
    case STORAGE_CLASS_GLOBAL:
    case STORAGE_CLASS_EXTERN:
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
    // 局部数组变量没有初始化
    *tree = NULL;
  } else
    t = parse_scalar_declaration(var_name, primitive_type, composite_type, storage_class, tree);

  return (t);
}

// 这里用于比较同时用 extern 声明的变量和 global 变量
// 这俩名字一样的情况下
// 用于将 extern 声明的变量变成 global
// return 1 为 true, 0 为 false
int is_new_symbol(
  struct SymbolTable *t,
  int storage_class,
  int primitive_type,
  struct SymbolTable *composite_type
) {
  if (!t) return 1;

  if ((t->storage_class == STORAGE_CLASS_GLOBAL && storage_class == STORAGE_CLASS_EXTERN) ||
      (t->storage_class == STORAGE_CLASS_EXTERN && storage_class == STORAGE_CLASS_GLOBAL)) {

    // 普通类型的比较
    if (primitive_type != t->primitive_type)
      error_with_message("Type mismatch between global/extern", t->name);

    // struct/union 的比较
    if (primitive_type >= PRIMITIVE_STRUCT &&
        composite_type != t->composite_type)
      error_with_message("Type mismatch between global/extern", t->name);

    // 如果类型比较后一样，将 extern 转为 global
    t->storage_class = STORAGE_CLASS_GLOBAL;
    return 0;
  }

  error_with_message("Duplicate global variable declaration", t->name);
  return -1;
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
  if (storage_class)
    error("Can't have extern in a typedef declaration");

  // 解析 xxx
  // 如果重复定义就报错
  if (find_typedef_symbol(text_buffer))
    error_with_message("Redefinition of typedef", text_buffer);

  // 解析 '*'
  primitive_type = convert_multiply_token_2_primitive_type(primitive_type);

  // 如果没有重复定义就加入 typedef symbol table 链表
  add_typedef_symbol(text_buffer, primitive_type, 0, 0, *composite_type);

  // 跳过 ';'
  scan(&token_from_file);
  return (primitive_type);
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
  return (t->primitive_type);
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
        if (*storage_class == STORAGE_CLASS_STATIC)
          error("Illegal to have extern and static at the same time");
        *storage_class = STORAGE_CLASS_EXTERN;
        scan(&token_from_file);
        break;
      case TOKEN_STATIC:
        if (*storage_class == STORAGE_CLASS_LOCAL)
          error("Compiler doesn't support static local declarations");
        else if (*storage_class == STORAGE_CLASS_EXTERN)
          error("Illegal to have extern and static at the same time");
        *storage_class = STORAGE_CLASS_STATIC;
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

  return (new_type);
}

struct SymbolTable *parse_function_declaration(
  int primitive_type,
  char *function_name,
  struct SymbolTable *composite_type,
  int storage_class
) {
  struct ASTNode *tree, *final_statement;
  int end_label = 0, parameter_count = 0;
  struct SymbolTable *old_function_symbol_table = find_symbol(function_name),
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
      0,
      STORAGE_CLASS_GLOBAL,
      NULL,
      end_label);
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
    return (old_function_symbol_table);

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
    old_function_symbol_table,
    composite_type);

  // 优化 ast tree
  tree = optimise(tree);

  if (output_dump_ast) {
    dump_ast(tree, NO_LABEL, 0);
    fprintf(stdout, "\n\n");
  }

  interpret_ast_with_register(tree, NO_LABEL, NO_LABEL, NO_LABEL, 0);

  clear_local_symbol_table();

  return (old_function_symbol_table);
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
  int end_token_2,
  struct ASTNode **glue_tree
) {
  int init_primitive_type, primitive_type;
  struct SymbolTable *t;
  struct ASTNode *tree;

  *glue_tree = NULL;

  // 检查是否是复合类型
  if ((init_primitive_type
    = convert_token_2_primitive_type(
      composite_type, &storage_class)) == -1)
    return (init_primitive_type);

  // 解析定义列表
  while(1) {
    // 检查 symbol 是否为指针
    primitive_type = convert_multiply_token_2_primitive_type(init_primitive_type);

    // 解析 symbol
    t = parse_symbol_declaration(primitive_type, *composite_type, storage_class, &tree);

    // 函数不在初始化变量的范围，直接返回
    if (t->structural_type == STRUCTURAL_FUNCTION) {
      if (storage_class != STORAGE_CLASS_GLOBAL &&
          storage_class != STORAGE_CLASS_STATIC)
        error("Function definition not at global level");
      return (primitive_type);
    }

    // 解析 local 变量，粘接 ast tree
    *glue_tree = (*glue_tree)
      ? create_ast_node(AST_GLUE, PRIMITIVE_NONE, *glue_tree, NULL, tree, 0, NULL, NULL)
      : tree;

    if (token_from_file.token == end_token ||
        token_from_file.token == end_token_2)
      return (primitive_type);

    verify_comma();
  }
}

void parse_global_declaration() {
  struct SymbolTable *composite_type;
  struct ASTNode *tree;

  // 解析声明的全局变量/函数
  while (token_from_file.token != TOKEN_EOF) {
    parse_declaration_list(
      &composite_type,
      STORAGE_CLASS_GLOBAL,
      TOKEN_SEMICOLON,
      TOKEN_EOF,
      &tree);

    if (token_from_file.token == TOKEN_SEMICOLON)
      scan(&token_from_file);
  }
}
