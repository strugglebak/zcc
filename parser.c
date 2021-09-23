#include <stdio.h>
#include <stdlib.h>
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
    error_with_digital("Token with no precedence in op_precedence:", operation_in_token);
  if (!precedence)
    error_with_digital("Syntax error, token", operation_in_token);
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
  int symbol_table_index;

  switch (token_from_file.token) {
    case TOKEN_STRING_LITERAL:
      // 先生成全局 string 的汇编代码，然后再解析 ast，因为这个 string 是要放在 .s 文件的最前面
      symbol_table_index = generate_global_string_code(text_buffer);
      // symbole_table_index 留给 generator 解析时用
      node = create_ast_leaf(AST_STRING_LITERAL, PRIMITIVE_CHAR_POINTER, symbol_table_index);
      break;
    // 解析类似   char j; j= 2; 这样的语句需要考虑的情况
    // 即把 2 这个 int 数值转换成 char 类型的
    case TOKEN_INTEGER_LITERAL:
      node = create_ast_leaf(
        AST_INTEGER_LITERAL,
        token_from_file.integer_value >= 0 && token_from_file.integer_value < 256
          ? PRIMITIVE_CHAR
          : PRIMITIVE_INT,
        token_from_file.integer_value);
      break;

    case TOKEN_IDENTIFIER:
      return convert_postfix_expression_2_ast();

    case TOKEN_LEFT_PAREN:
      // 解析 e= (a+b) * (c+d); 类似的语句
      // 如果表达式以 ( 开头，略过它
      scan(&token_from_file);
      node = converse_token_2_ast(0);
      verify_right_paren();
      return node;
    default:
      error_with_digital("Expecting a primary expression, got token", token_from_file.token);
  }

  // 扫描下一个，继续判断
  scan(&token_from_file);

  return node;
}

// 将 token 中的 + - * / 等转换成 ast 中的类型
int convert_token_operation_2_ast_operation(int operation_in_token) {
  if (operation_in_token > TOKEN_EOF && operation_in_token < TOKEN_INTEGER_LITERAL)
    return operation_in_token;
  error_with_digital("Syntax error, token", operation_in_token);
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
    TOKEN_RIGHT_BRACKET == node_operation_type
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
    left = create_ast_node(ast_operation_type, left->primitive_type, left, NULL, right, 0);

    node_operation_type = token_from_file.token;
    if (TOKEN_SEMICOLON == node_operation_type ||
      TOKEN_EOF == node_operation_type ||
      TOKEN_RIGHT_PAREN == node_operation_type ||
      TOKEN_RIGHT_BRACKET == node_operation_type
      ) break;
  }

  // 返回这颗构建的树
  left->rvalue = 1;
  return left;
}

struct ASTNode *convert_function_call_2_ast() {
  struct ASTNode *tree;
  int symbol_table_index;

  // 解析类似于 xxx(1); 这样的函数调用

  // 检查是否未声明
  if ((symbol_table_index = find_symbol(text_buffer)) == -1)
    error_with_message("Undeclared function", text_buffer);

  // 解析左 (
  verify_left_paren();

  // 解析括号中的语句
  tree = converse_token_2_ast(0);

  // 保存函数返回的类型作为这个 node 的类型
  // 保存函数名在这个 symbol table 中的 index 索引
  tree = create_ast_left_node(
    AST_FUNCTION_CALL,
    symbol_table[symbol_table_index].primitive_type,
    tree,
    symbol_table_index);

  // 解析右 )
  verify_right_paren();

  return tree;
}

struct ASTNode *convert_array_access_2_ast() {
  struct ASTNode *left, *right;
  struct SymbolTable t;
  int symbol_table_index = find_symbol(text_buffer);

  // 解析类似于 x = a[12];
  if (symbol_table_index == -1 ||
      symbol_table[symbol_table_index].structural_type
        != STRUCTURAL_ARRAY)
    error_with_message("Undeclared array", text_buffer);

  // 为数组变量创建一个子节点，这个子节点指向数组的基
  t = symbol_table[symbol_table_index];
  left = create_ast_leaf(AST_IDENTIFIER_ADDRESS, t.primitive_type, symbol_table_index);

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
  left = create_ast_node(AST_PLUS, t.primitive_type, left, NULL, right, 0);
  // 这个时候也必须要解除引用，因为可能后面会有 a[12] = 100; 这样的语句出现，所以把它看作左值 lvalue
  left = create_ast_left_node(AST_DEREFERENCE_POINTER, value_at(left->primitive_type), left, 0);

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
        0);
      break;
    case TOKEN_MINUS:
      // 解析类似 x = -y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      tree->rvalue = 1;
      // 有可能 y 是 char(unsigned)，而 x 是 int 型的，所以需要做一个拓展让其变成有符号(signed)的
      // 并且 unsigned 值不能取负数
      tree = modify_type(tree, PRIMITIVE_INT, 0);
      tree = create_ast_left_node(AST_NEGATE, tree->primitive_type, tree, 0);
      break;
    case TOKEN_INVERT:
      // 解析类似 x = ~y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      tree->rvalue = 1;
      tree = create_ast_left_node(AST_INVERT, tree->primitive_type, tree, 0);
      break;
    case TOKEN_LOGIC_NOT:
      // 解析类似 x = !y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      tree->rvalue = 1;
      tree = create_ast_left_node(AST_LOGIC_NOT, tree->primitive_type, tree, 0);
      break;
    case TOKEN_INCREASE:
      // 解析类似 x = ++y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      if (tree->operation != AST_IDENTIFIER)
        error("++ operator must be followed by an identifier");

      tree = create_ast_left_node(AST_PRE_INCREASE, tree->primitive_type, tree, 0);
      break;
    case TOKEN_DECREASE:
      // 解析类似 x = --y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast();

      if (tree->operation != AST_IDENTIFIER)
        error("-- operator must be followed by an identifier");

      tree = create_ast_left_node(AST_PRE_DECREASE, tree->primitive_type, tree, 0);
      break;
    default:
      tree = create_ast_node_from_expression();
  }

  return tree;
}

struct ASTNode *convert_postfix_expression_2_ast() {
  struct ASTNode *tree;
  struct SymbolTable t;
  int symbol_table_index;

  // 解析类似的语句时需要注意的问题，即区别是变量名还是函数调用还是数组访问
  //  x = fred + jim;
  //  x = fred(5) + jim;
  //  x = a[12];

  // 扫描下一个，继续判断
  scan(&token_from_file);

  // 如果是左 (，则直接看作是函数调用
  if (token_from_file.token == TOKEN_LEFT_PAREN)
    return convert_function_call_2_ast();

  // 如果是左 [，则直接看作是访问数组
  if (token_from_file.token == TOKEN_LEFT_BRACKET)
    return convert_array_access_2_ast();

  // 检查标识符是否存在
  if ((symbol_table_index = find_symbol(text_buffer)) == -1 ||
      symbol_table[symbol_table_index].structural_type != STRUCTURAL_VARIABLE)
        error_with_message("Unknown variable", text_buffer);

  t = symbol_table[symbol_table_index];
  switch (token_from_file.token) {
    case TOKEN_INCREASE:
      // 解析类似 x = y++; 语句
      scan(&token_from_file);
      tree = create_ast_leaf(AST_POST_INCREASE, t.primitive_type, symbol_table_index);
      break;
    case TOKEN_DECREASE:
      // 解析类似 x = y--; 语句
      scan(&token_from_file);
      tree = create_ast_leaf(AST_POST_DECREASE, t.primitive_type, symbol_table_index);
      break;
    default:
      tree = create_ast_leaf(AST_IDENTIFIER, t.primitive_type, symbol_table_index);
      break;
  }

  return tree;
}
