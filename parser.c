#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "definations.h"
#include "data.h"
#include "ast.h"
#include "scan.h"
#include "helper.h"
#include "symbol_table.h"
#include "types.h"
#include "parser.h"
#include "generator.h"

static int operation_precedence_array[] = {
  0, 10, 20, 30,  // EOF = || &&
  40, 50, 60,     // | ^ &
  70, 70,         // == !=
  80, 80, 80, 80, // < > <= >=
  90, 90,         // << >>
  100, 100,       // + -
  110, 110        // * /
};

static int check_right_associative(int token) {
  return token == TOKEN_ASSIGN ? 1 : 0;
}

// 确定操作符的优先级
static int operation_precedence(int operation_in_token) {
  int precedence = operation_precedence_array[operation_in_token];
  if (operation_in_token > TOKEN_DIVIDE)
    error_with_message(
      "Token with no precedence in op_precedence",
       token_string[operation_in_token]);
  if (!precedence)
    error_with_message(
      "Syntax error, token",
       token_string[operation_in_token]);
  return precedence;
}

// 逐个字的读取文件，并将文件中读取到的数字字符构建成一颗树
// primary_expression
//         : IDENTIFIER
//         | CONSTANT
//         | STRING_LITERAL
//         | '(' expression ')'
//         ;

// postfix_expression
//         : primary_expression
//         | postfix_expression '[' expression ']'
//         | postfix_expression '(' expression ')'
//         | postfix_expression '++'
//         | postfix_expression '--'
//         ;

// prefix_expression
//         : postfix_expression
//         | '++' prefix_expression
//         | '--' prefix_expression
//         | prefix_operator prefix_expression
//         ;

// prefix_operator
//         : '&'
//         | '*'
//         | '-'
//         | '+'
//         | '~'
//         | '!'
//         ;

// multiplicative_expression
//         : prefix_expression
//         | multiplicative_expression '*' prefix_expression
//         | multiplicative_expression '/' prefix_expression
//         | multiplicative_expression '%' prefix_expression
//         ;
static struct ASTNode *create_ast_node_from_expression() {
  struct ASTNode *node;
  int label;

  switch (token_from_file.token) {
    case TOKEN_STRING_LITERAL:
      // 先生成全局 string 的汇编代码，然后再解析 ast，因为这个 string 是要放在 .s 文件的最前面
      label = generate_global_string_code(text_buffer);
      // symbole_table_index 留给 generator 解析时用
      node = create_ast_leaf(
        AST_STRING_LITERAL,
        pointer_to(PRIMITIVE_CHAR),
        label,
        NULL);
      break;
    // 解析类似   char j; j= 2; 这样的语句需要考虑的情况
    // 即把 2 这个 int 数值转换成 char 类型的
    case TOKEN_INTEGER_LITERAL:
      node = create_ast_leaf(
        AST_INTEGER_LITERAL,
        token_from_file.integer_value >= 0 && token_from_file.integer_value < 256
          ? PRIMITIVE_CHAR
          : PRIMITIVE_INT,
        token_from_file.integer_value,
        NULL);
      break;

    case TOKEN_IDENTIFIER:
      return convert_postfix_expression_2_ast();

    case TOKEN_LEFT_PAREN:
      // 解析 e= (a+b) * (c+d); 类似的语句
      // 如果表达式以 ( 开头，跳过它
      scan(&token_from_file);
      node = converse_token_2_ast(0);
      verify_right_paren();
      return node;
    default:
      error_with_message("Expecting a primary expression, got token", token_from_file.token_string);
  }

  // 扫描下一个，继续判断
  scan(&token_from_file);

  return node;
}

// expression_list: <null>
//        | expression
//        | expression ',' expression_list
//        ;
// 解析类似 function(expr1, expr2, expr3, expr4); 的语句
// 然后创建如下形式的 tree
//                AST_FUNCCALL
//                 /
//             AST_GLUE
//              /   \
//          AST_GLUE  expr4(4)
//           /   \
//       AST_GLUE  expr3(3)
//        /   \
//    AST_GLUE  expr2(2)
//    /    \
//  NULL  expr1(1)
// 确保处理顺序为 expr4 expr3 expr2 expr1，以便生成汇编代码时压栈顺序正确
struct ASTNode *parse_expression_list(int end_token_type) {
  struct ASTNode *tree = NULL, *right = NULL;
  int expression_count = 0;

  while (token_from_file.token != end_token_type) {
    right = converse_token_2_ast(0);
    expression_count++;

    // 开始生成一颗树
    tree = create_ast_node(
      AST_GLUE,
      PRIMITIVE_NONE,
      tree,
      NULL,
      right,
      expression_count,
      NULL);

    // 循环到 end_token_type 为止
    if (token_from_file.token == end_token_type) break;

    // 这里必须检查 ','
    verify_comma();
  }

  return tree;
}

// 将 token 中的 + - * / 等转换成 ast 中的类型
int convert_token_operation_2_ast_operation(int operation_in_token) {
  if (operation_in_token > TOKEN_EOF && operation_in_token < TOKEN_INTEGER_LITERAL)
    return operation_in_token;
  error_with_message("Syntax error, token", token_string[operation_in_token]);
}

/**
 * 这里就将文件中解析到的 token 转化成 ast
 * 注意这里相当于 后序遍历
 * 主要是能将类似于 2 * 3 + 4 * 5 之类的表达式转换成如下形式的树结构
 *         +
 *        / \
 *       /   \
 *      /     \
 *     *       *
 *    / \     / \
 *   2   3   4   5
 * 注意这里是有 「优先级」的，目前来说 * / 符号的优先级最高
 * 注意这里用 pratt 解析的方式进行
*/
struct ASTNode *converse_token_2_ast(int previous_token_precedence) {
  struct ASTNode *left, *right, *left_temp, *right_temp;
  int node_operation_type;
  int ast_operation_type;

  // 初始化左子树
  left = convert_prefix_expression_2_ast();

  // 到这里说明已经遍历到文件尾，可以直接 return
  node_operation_type = token_from_file.token;
  // 如果遇到分号，也可以直接 return
  // 如果遇到')' 说明这个节点已经构建完成，直接返回 left
  if (TOKEN_SEMICOLON == node_operation_type ||
    TOKEN_EOF == node_operation_type ||
    TOKEN_RIGHT_PAREN == node_operation_type ||
    TOKEN_RIGHT_BRACKET == node_operation_type ||
    TOKEN_COMMA == node_operation_type ||
    TOKEN_COLON == node_operation_type
    ) {
      left->rvalue = 1;
      return left;
    }

  // 如果这次扫描得到的操作符比之前的优先级高，
  // 或者它是个右结合操作符 =，并且优先级和上一个操作符相同
  // 则进行树的循环构建
  while(
    (operation_precedence(node_operation_type)
      > previous_token_precedence) ||
    (check_right_associative(node_operation_type)
      && operation_precedence(node_operation_type) == previous_token_precedence)
  ) {
    // 继续扫描
    scan(&token_from_file);
    // 开始构建右子树
    right = converse_token_2_ast(operation_precedence(node_operation_type));

    // 将 Token 操作符类型转换成 AST 操作符类型
    ast_operation_type = convert_token_operation_2_ast_operation(node_operation_type);
    right->rvalue = 1;
    if (ast_operation_type == AST_ASSIGN) {
      // 确保 right 和 left 的类型匹配
      right = modify_type(right, left->primitive_type, 0);
      if (!left) error("Incompatible expression in assignment");

      // 交换 left 和 right，确保 right 汇编语句能在 left 之前生成
      left_temp = left; left = right; right = left_temp;
    } else {
      // 如果不是赋值操作，那么显然所有的 tree node 都是 rvalue 右值
      left->rvalue = 1;

      // 检查 left 和 right 节点的 primitive_type 是否兼容
      // 这里同时判断了指针的类型
      left_temp = modify_type(left, right->primitive_type, ast_operation_type);
      right_temp = modify_type(right, left->primitive_type, ast_operation_type);
      if (!left_temp && !right_temp) error("Incompatible types in binary expression");
      if (left_temp) left = left_temp;
      if (right_temp) right = right_temp;
    }


    // 开始构建左子树
    left = create_ast_node(
      ast_operation_type,
      left->primitive_type,
      left,
      NULL,
      right,
      0,
      NULL);

    node_operation_type = token_from_file.token;
    if (TOKEN_SEMICOLON == node_operation_type ||
      TOKEN_EOF == node_operation_type ||
      TOKEN_RIGHT_PAREN == node_operation_type ||
      TOKEN_RIGHT_BRACKET == node_operation_type ||
      TOKEN_COMMA == node_operation_type ||
      TOKEN_COLON == node_operation_type
      ) break;
  }

  // 返回这颗构建的树
  left->rvalue = 1;
  return left;
}

struct ASTNode *convert_function_call_2_ast() {
  struct ASTNode *tree;
  struct SymbolTable *t = find_symbol(text_buffer);
  // 解析类似于 xxx(1); 这样的函数调用

  // 检查是否未声明
  if (!t || t->structural_type != STRUCTURAL_FUNCTION)
    error_with_message("Undeclared function", text_buffer);

  // 解析左 (
  verify_left_paren();

  // 解析括号中的语句
  tree = parse_expression_list(TOKEN_RIGHT_PAREN);

  // 保存函数返回的类型作为这个 node 的类型
  // 保存函数名在这个 symbol table 中的 index 索引
  tree = create_ast_left_node(
    AST_FUNCTION_CALL,
    t->primitive_type,
    tree,
    0,
    t);

  // 解析右 )
  verify_right_paren();

  return tree;
}

struct ASTNode *convert_array_access_2_ast() {
  struct ASTNode *left, *right;
  struct SymbolTable *t = find_symbol(text_buffer);

  // 解析类似于 x = a[12];
  if (!t || t->structural_type != STRUCTURAL_ARRAY)
    error_with_message("Undeclared array", text_buffer);

  // 为数组变量创建一个子节点，这个子节点指向数组的基
  left = create_ast_leaf(AST_IDENTIFIER_ADDRESS, t->primitive_type, 0, t);

  // 跳过 [
  scan(&token_from_file);

  // 解析在中括号之间的表达式
  right = converse_token_2_ast(0);

  // 检查 ]
  verify_right_bracket();

  // 检查中括号中间的表达式中的 type 是否是一个 int 数值
  if (!check_int_type(right->primitive_type))
    error("Array index is not of integer type");

  // 对于 int a[20]; 数组索引 a[12] 来说，需要用 int 类型(4) 来扩展 数组索引(12)
  // 以便在生成汇编代码时增加偏移量
  right = modify_type(right, left->primitive_type, AST_PLUS);

  // 返回一个 AST 树，其中数组的基添加了偏移量
  left = create_ast_node(AST_PLUS, t->primitive_type, left, NULL, right, 0, NULL);
  // 这个时候也必须要解除引用，因为可能后面会有 a[12] = 100; 这样的语句出现，所以把它看作左值 lvalue
  left = create_ast_left_node(AST_DEREFERENCE_POINTER, value_at(left->primitive_type), left, 0, NULL);

  return left;
}

struct ASTNode *convert_member_access_2_ast(int with_pointer) {
  // 解析类似于 y = xxx.a; 或者 y = xxx->a; 这样的语句
  struct ASTNode *left, *right;
  struct SymbolTable *var_pointer, *type_pointer, *member;

  // 检查 identifier 是否用 struct 定义过
  if (!(var_pointer = find_symbol(text_buffer)))
    error_with_message("Undeclared variable", text_buffer);
  // 定义过用的是 'xxx->a' 的用法，检查 xxx 是不是指针类型
  if (with_pointer &&
      var_pointer->primitive_type != pointer_to(PRIMITIVE_STRUCT) &&
      var_pointer->primitive_type != pointer_to(PRIMITIVE_UNION))
    error_with_message("Variable is not a pointer type", text_buffer);
  // 定义过用的是 'xxx.a' 的用法，检查 xxx 是不是结构体类型
  if (!with_pointer &&
      var_pointer->primitive_type != PRIMITIVE_STRUCT &&
      var_pointer->primitive_type != PRIMITIVE_UNION)
    error_with_message("Variable is not a struct type", text_buffer);

  // 创建 left 节点
  left = with_pointer
    // 指针
    ? create_ast_leaf(AST_IDENTIFIER, pointer_to(PRIMITIVE_STRUCT), 0, var_pointer)
    // 结构体
    : create_ast_leaf(AST_IDENTIFIER_ADDRESS, var_pointer->primitive_type, 0, var_pointer);
  // 它是一个右值
  left->rvalue = 1;

  type_pointer = var_pointer->composite_type;

  // 跳过 '.' 或者 '->'
  scan(&token_from_file);
  verify_identifier();

  // 在 type_pointer 指向的 symbol table 中寻找 'xxx.a' 或者 'xxx->a' 中的 'a'
  for (member = type_pointer->member; member; member = member->next)
    if (!strcmp(member->name, text_buffer)) break;

  // 没找到 'a' 直接退出
  if (!member) error_with_message("No member found in struct/union", text_buffer);

  // 找到了 'a'，先创建一个右节点，值是 'a' 所在的成员的 offset
  right = create_ast_leaf(AST_INTEGER_LITERAL, PRIMITIVE_INT, member->position, NULL);

  // 返回一个 AST 树，其中 struct 的基添加了 member 的偏移量
  left = create_ast_node(AST_PLUS, pointer_to(member->primitive_type), left, NULL, right, 0, NULL);
  // 这个时候也必须要解除引用，因为可能后面会有 xxx.a = 100; 这样的语句出现，所以把它看作左值 lvalue
  left = create_ast_left_node(AST_DEREFERENCE_POINTER, member->primitive_type, left, 0, NULL);
  return left;
}

struct ASTNode *convert_prefix_expression_2_ast() {
  struct ASTNode *tree;
  switch (token_from_file.token) {
    case TOKEN_AMPERSAND:
      // 解析类似于 x= &&&y;
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      if (tree->operation != AST_IDENTIFIER)
        error("& operator must be followed by an identifier");
      tree->operation = AST_IDENTIFIER_ADDRESS;
      tree->primitive_type = pointer_to(tree->primitive_type);
      break;
    case TOKEN_MULTIPLY:
      // 解析类似于 x= ***y;
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      if (tree->operation != AST_IDENTIFIER &&
        tree->operation != AST_DEREFERENCE_POINTER)
        error("* operator must be followed by an identifier or *");
      // 生成一个父级节点
      tree = create_ast_left_node(
        AST_DEREFERENCE_POINTER,
        value_at(tree->primitive_type),
        tree,
        0,
        NULL);
      break;
    case TOKEN_MINUS:
      // 解析类似 x = -y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      tree->rvalue = 1;
      // 有可能 y 是 char(unsigned)，而 x 是 int 型的，所以需要做一个拓展让其变成有符号(signed)的
      // 并且 unsigned 值不能取负数
      tree = modify_type(tree, PRIMITIVE_INT, 0);
      tree = create_ast_left_node(AST_NEGATE, tree->primitive_type, tree, 0, NULL);
      break;
    case TOKEN_INVERT:
      // 解析类似 x = ~y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      tree->rvalue = 1;
      tree = create_ast_left_node(AST_INVERT, tree->primitive_type, tree, 0, NULL);
      break;
    case TOKEN_LOGIC_NOT:
      // 解析类似 x = !y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      tree->rvalue = 1;
      tree = create_ast_left_node(AST_LOGIC_NOT, tree->primitive_type, tree, 0, NULL);
      break;
    case TOKEN_INCREASE:
      // 解析类似 x = ++y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      if (tree->operation != AST_IDENTIFIER)
        error("++ operator must be followed by an identifier");

      tree = create_ast_left_node(AST_PRE_INCREASE, tree->primitive_type, tree, 0, NULL);
      break;
    case TOKEN_DECREASE:
      // 解析类似 x = --y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      if (tree->operation != AST_IDENTIFIER)
        error("-- operator must be followed by an identifier");

      tree = create_ast_left_node(AST_PRE_DECREASE, tree->primitive_type, tree, 0, NULL);
      break;
    default:
      tree = create_ast_node_from_expression();
  }

  return tree;
}

struct ASTNode *convert_postfix_expression_2_ast() {
  struct ASTNode *tree;
  struct SymbolTable *t = NULL, *enum_pointer;

  // 解析类似的语句时需要注意的问题，即区别是变量名还是函数调用还是数组访问
  //  x = fred + jim;
  //  x = fred(5) + jim;
  //  x = a[12];

  // 先判断 fred 或者 jim 是不是 enum 变量
  // 如果是得先返回一个 ast node
  if ((enum_pointer = find_enum_value_symbol(text_buffer))) {
    scan(&token_from_file);
    return create_ast_leaf(AST_INTEGER_LITERAL, PRIMITIVE_INT, enum_pointer->position, NULL);
  }

  // 扫描下一个，继续判断
  scan(&token_from_file);

  // 如果是左 (，则直接看作是函数调用
  if (token_from_file.token == TOKEN_LEFT_PAREN)
    return convert_function_call_2_ast();

  // 如果是左 [，则直接看作是访问数组
  if (token_from_file.token == TOKEN_LEFT_BRACKET)
    return convert_array_access_2_ast();

  // 如果是 '.' 或者 '->' 看作是访问 struct/union
  if (token_from_file.token == TOKEN_DOT)
    return convert_member_access_2_ast(0);
  if (token_from_file.token == TOKEN_ARROW)
    return convert_member_access_2_ast(1);

  // 检查标识符是否存在
  t = find_symbol(text_buffer);
  if (!t || t->structural_type != STRUCTURAL_VARIABLE)
    error_with_message("Unknown variable", text_buffer);

  switch (token_from_file.token) {
    case TOKEN_INCREASE:
      // 解析类似 x = y++; 语句
      scan(&token_from_file);
      tree = create_ast_leaf(AST_POST_INCREASE, t->primitive_type, 0, t);
      break;
    case TOKEN_DECREASE:
      // 解析类似 x = y--; 语句
      scan(&token_from_file);
      tree = create_ast_leaf(AST_POST_DECREASE, t->primitive_type, 0, t);
      break;
    default:
      tree = create_ast_leaf(AST_IDENTIFIER, t->primitive_type, 0, t);
      break;
  }

  return tree;
}
