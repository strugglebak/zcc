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
#include "declaration.h"

static struct ASTNode *parse_paren_expression(int previous_token_precedence);

static int operation_precedence_array[] = {
  0, 10,              // EOF =
  10, 10, 10, 10, 10, // += -= *= /= %=
  15,                 // \? 三元操作符
  20, 30,             // || &&
  40, 50, 60,         // | ^ &
  70, 70,             // == !=
  80, 80, 80, 80,     // < > <= >=
  90, 90,             // << >>
  100, 100,           // + -
  110, 110            // * /
};

static int check_right_associative(int token) {
  return (token >= TOKEN_ASSIGN && token <= TOKEN_ASSGIN_DIVIDE ? 1 : 0);
}

// 将 token 中的 + - * / 等转换成 ast 中的类型
static int convert_token_operation_2_ast_operation(int operation_in_token) {
  if (operation_in_token > TOKEN_EOF && operation_in_token <= TOKEN_MOD)
    return (operation_in_token);
  error_with_message("Syntax error, token", token_strings[operation_in_token]);
  return (0);
}

// 确定操作符的优先级
static int operation_precedence(int operation_in_token) {
  int precedence = operation_precedence_array[operation_in_token];
  if (operation_in_token > TOKEN_MOD)
    error_with_message(
      "Token with no precedence in op_precedence",
       token_strings[operation_in_token]);
  if (!precedence)
    error_with_message(
      "Syntax error, token",
       token_strings[operation_in_token]);
  return (precedence);
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
static struct ASTNode *create_ast_node_from_expression(int previous_token_precedence) {
  struct ASTNode *node;
  struct SymbolTable *composite_type, *var_pointer, *enum_pointer;
  int label;
  int primitive_type = 0;
  int primitive_type_size, storage_class;

  switch (token_from_file.token) {
    case TOKEN_STATIC:
    case TOKEN_EXTERN:
      error("Compiler doesn't support static or extern local declarations");
    case TOKEN_SIZEOF:
      // 解析类似于 int a = 1 + sizeof(char); 这样的语句
      // 跳过 'sizeof'
      scan(&token_from_file);
      if (token_from_file.token != TOKEN_LEFT_PAREN)
        error("Left parenthesis expected after sizeof");
      // 跳过 '('
      scan(&token_from_file);

      // 也可能会解析 sizeof(char *)
      primitive_type
        = convert_multiply_token_2_primitive_type(
          convert_token_2_primitive_type(
            &composite_type,
            &storage_class
          )
        );
      primitive_type_size = get_primitive_type_size(primitive_type, composite_type);
      verify_right_paren();
      return (
        create_ast_leaf(
          AST_INTEGER_LITERAL,
          PRIMITIVE_INT,
          primitive_type_size,
          NULL,
          NULL)
      );

    case TOKEN_STRING_LITERAL:
      // 先生成全局 string 的汇编代码，然后再解析 ast，因为这个 string 是要放在 .s 文件的最前面
      label = generate_global_string_code(text_buffer, 0);

      // 支持连续的字符串赋值，类似 char *a = "hello" "world";
      while(1) {
        scan(&look_ahead_token);
        if (look_ahead_token.token != TOKEN_STRING_LITERAL)
          break;
        generate_global_string_code(text_buffer, 1);
        scan(&token_from_file);
      }

      generate_global_string_code_end();

      // symbole_table_index 留给 generator 解析时用
      node = create_ast_leaf(
        AST_STRING_LITERAL,
        pointer_to(PRIMITIVE_CHAR),
        label,
        NULL,
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
        NULL,
        NULL);
      break;

    case TOKEN_IDENTIFIER:
      enum_pointer = find_enum_value_symbol(text_buffer);
      if (enum_pointer) {
        node = create_ast_leaf(
          AST_INTEGER_LITERAL,
          PRIMITIVE_INT,
          enum_pointer->symbol_table_position,
          NULL,
          NULL);
        break;
      }
      var_pointer = find_symbol(text_buffer);
      if (!var_pointer)
        error_with_message("Unknown variable or function", text_buffer);
      switch (var_pointer->structural_type) {
        case STRUCTURAL_VARIABLE:
          node = create_ast_leaf(
            AST_IDENTIFIER,
            var_pointer->primitive_type,
            0,
            var_pointer,
            var_pointer->composite_type);
          break;
        case STRUCTURAL_ARRAY:
          node = create_ast_leaf(
            AST_IDENTIFIER_ADDRESS,
            var_pointer->primitive_type,
            0,
            var_pointer,
            var_pointer->composite_type);
          node->rvalue = 1;
          break;
        case STRUCTURAL_FUNCTION:
          scan(&token_from_file);
          if (token_from_file.token != TOKEN_LEFT_PAREN)
            error_with_message("Function name used without parentheses", text_buffer);
          return (convert_function_call_2_ast());
        default:
          error_with_message("Identifier not a scalar or array variable", text_buffer);
      }
      break;

    case TOKEN_LEFT_PAREN:
      // 解析 e= (a+b) * (c+d); 类似的语句
      return (parse_paren_expression(previous_token_precedence));

    default:
      error_with_message("Expecting a primary expression, got token", token_from_file.token_string);
  }

  // 扫描下一个，继续判断
  scan(&token_from_file);

  return (node);
}

// 解析 (expression) 即括号中的语句
static struct ASTNode *parse_paren_expression(int previous_token_precedence) {
  struct ASTNode *tree;
  struct SymbolTable *composite_type = NULL;
  int primitive_type = 0;

  // 跳过 '('
  scan(&token_from_file);

  switch (token_from_file.token) {
    case TOKEN_IDENTIFIER:
      if (!find_typedef_symbol(text_buffer)) {
        tree = converse_token_2_ast(0);
        break;
      }
    case TOKEN_VOID:
    case TOKEN_CHAR:
    case TOKEN_INT:
    case TOKEN_LONG:
    case TOKEN_STRUCT:
    case TOKEN_UNION:
    case TOKEN_ENUM:
      // 获取到强制类型转换后的 type
      primitive_type = convert_type_casting_token_2_primitive_type(&composite_type);
      // 跳过 ')'
      verify_right_paren();
    default:
      tree = converse_token_2_ast(previous_token_precedence);
  }

  if (!primitive_type)
    // 没有强制类型转换这样也要跳过 ')'
    verify_right_paren();
  else
    // 否则创建一个 强制类型转换的 tree
    tree = create_ast_left_node(AST_TYPE_CASTING, primitive_type, tree, 0, NULL, composite_type);

  return (tree);
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
struct ASTNode *parse_expression_list(int end_token) {
  struct ASTNode *tree = NULL, *right = NULL;
  int expression_count = 0;

  while (token_from_file.token != end_token) {
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
      NULL,
      NULL);

    // 循环到 end_token 为止
    if (token_from_file.token == end_token) break;

    // 这里必须检查 ','
    verify_comma();
  }

  return (tree);
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
  left = convert_prefix_expression_2_ast(previous_token_precedence);

  // 到这里说明已经遍历到文件尾，可以直接 return
  node_operation_type = token_from_file.token;
  // 如果遇到分号，也可以直接 return
  // 如果遇到')' 说明这个节点已经构建完成，直接返回 left
  if (TOKEN_SEMICOLON == node_operation_type ||
    TOKEN_EOF == node_operation_type ||
    TOKEN_RIGHT_PAREN == node_operation_type ||
    TOKEN_RIGHT_BRACKET == node_operation_type ||
    TOKEN_COMMA == node_operation_type ||
    TOKEN_COLON == node_operation_type ||
    TOKEN_RIGHT_BRACE == node_operation_type
    ) {
      left->rvalue = 1;
      return (left);
    }

  // 如果这次扫描得到的操作符比之前的优先级高，
  // 或者它是个右结合操作符 =，并且优先级和上一个操作符相同
  // 则进行树的循环构建
  while(
    (operation_precedence(node_operation_type) > previous_token_precedence) ||
    (check_right_associative(node_operation_type) &&
     operation_precedence(node_operation_type) == previous_token_precedence)
  ) {
    // 继续扫描
    scan(&token_from_file);
    // 开始构建右子树
    right = converse_token_2_ast(operation_precedence(node_operation_type));

    // 将 Token 操作符类型转换成 AST 操作符类型
    ast_operation_type = convert_token_operation_2_ast_operation(node_operation_type);

    switch (ast_operation_type) {
      case AST_TERNARY:
        // 解析三元表达式
        // ternary_expression:
        // logical_expression '?' true_expression ':' false_expression
        // ;
        verify_colon();
        left_temp = converse_token_2_ast(0);
        return (
          create_ast_node(
            AST_TERNARY,
            right->primitive_type,
            left, // logical_expression
            right, // true_expression
            left_temp, // false_expression
            0,
            NULL,
            right->composite_type)
        );
      case AST_ASSIGN:
        right->rvalue = 1;

        // 确保 right 和 left 的类型匹配
        right = modify_type(right, left->primitive_type, 0, left->composite_type);
        if (!right) error("Incompatible expression in assignment");

        // 交换 left 和 right，确保 right 汇编语句能在 left 之前生成
        left_temp = left; left = right; right = left_temp;
        break;
      default:
        // 如果不是赋值操作，那么显然所有的 tree node 都是 rvalue 右值
        left->rvalue = 1;
        right->rvalue = 1;

        // 检查 left 和 right 节点的 primitive_type 是否兼容
        // 这里同时判断了指针的类型
        left_temp = modify_type(left, right->primitive_type, ast_operation_type, right->composite_type);
        right_temp = modify_type(right, left->primitive_type, ast_operation_type, left->composite_type);
        if (!left_temp && !right_temp) error("Incompatible types in binary expression");
        if (left_temp) left = left_temp;
        if (right_temp) right = right_temp;
        break;
    }

    left = create_ast_node(
      ast_operation_type,
      left->primitive_type,
      left,
      NULL,
      right,
      0,
      NULL,
      left->composite_type);

    switch (ast_operation_type) {
      case AST_LOGIC_OR:
      case AST_LOGIC_AND:
      case AST_COMPARE_EQUALS:
      case AST_COMPARE_NOT_EQUALS:
      case AST_COMPARE_LESS_THAN:
      case AST_COMPARE_GREATER_THAN:
      case AST_COMPARE_LESS_EQUALS:
      case AST_COMPARE_GREATER_EQUALS:
        left->primitive_type = PRIMITIVE_INT;
    }

    node_operation_type = token_from_file.token;
    if (TOKEN_SEMICOLON == node_operation_type ||
      TOKEN_EOF == node_operation_type ||
      TOKEN_RIGHT_PAREN == node_operation_type ||
      TOKEN_RIGHT_BRACKET == node_operation_type ||
      TOKEN_COMMA == node_operation_type ||
      TOKEN_COLON == node_operation_type ||
      TOKEN_RIGHT_BRACE == node_operation_type
      ) break;
  }

  // 返回这颗构建的树
  left->rvalue = 1;
  return (left);
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
    t,
    t->composite_type);

  // 解析右 )
  verify_right_paren();

  return (tree);
}

struct ASTNode *convert_array_access_2_ast(struct ASTNode *left) {
  struct ASTNode *right;

  if (!check_pointer_type(left->primitive_type))
    error("Not an array or pointer");

  // 跳过 [
  scan(&token_from_file);

  // 解析在中括号之间的表达式
  right = converse_token_2_ast(0);

  // 检查 ]
  verify_right_bracket();

  // 检查中括号中间的表达式中的 type 是否是一个 int 数值
  if (!check_int_type(right->primitive_type))
    error("Array index is not of integer type");

  left->rvalue = 1;

  // 对于 int a[20]; 数组索引 a[12] 来说，需要用 int 类型(4) 来扩展 数组索引(12)
  // 以便在生成汇编代码时增加偏移量
  right = modify_type(right, left->primitive_type, AST_PLUS, left->composite_type);

  // 返回一个 AST 树，其中数组的基添加了偏移量
  left = create_ast_node(AST_PLUS, left->primitive_type, left, NULL, right, 0, NULL, left->composite_type);
  // 这个时候也必须要解除引用，因为可能后面会有 a[12] = 100; 这样的语句出现，所以把它看作左值 lvalue
  left = create_ast_left_node(AST_DEREFERENCE_POINTER, value_at(left->primitive_type), left, 0, NULL, left->composite_type);

  return (left);
}

struct ASTNode *convert_member_access_2_ast(int with_pointer, struct ASTNode *left) {
  // 解析类似于 y = xxx.a; 或者 y = xxx->a; 这样的语句
  struct ASTNode *right;
  struct SymbolTable *type_pointer, *member;

  if (with_pointer &&
      (left->primitive_type != pointer_to(PRIMITIVE_STRUCT) &&
       left->primitive_type != pointer_to(PRIMITIVE_UNION)))
    error("Expression is not a pointer to a struct/union");

  if (!with_pointer) {
    if (left->primitive_type == PRIMITIVE_STRUCT ||
        left->primitive_type == PRIMITIVE_UNION)
      left->operation = AST_IDENTIFIER_ADDRESS;
    else
      error("Expression is not a struct/union");
  }

  type_pointer = left->composite_type;

  // 跳过 '.' 或者 '->'
  scan(&token_from_file);
  verify_identifier();

  // 在 type_pointer 指向的 symbol table 中寻找 'xxx.a' 或者 'xxx->a' 中的 'a'
  for (member = type_pointer->member; member; member = member->next)
    if (!strcmp(member->name, text_buffer)) break;

  // 没找到 'a' 直接退出
  if (!member) error_with_message("No member found in struct/union", text_buffer);

  left->rvalue = 1;

  // 找到了 'a'，先创建一个右节点，值是 'a' 所在的成员的 offset
  right = create_ast_leaf(AST_INTEGER_LITERAL, PRIMITIVE_INT, member->symbol_table_position, NULL, NULL);

  // 返回一个 AST 树，其中 struct 的基添加了 member 的偏移量
  left = create_ast_node(AST_PLUS, pointer_to(member->primitive_type), left, NULL, right, 0, NULL, member->composite_type);
  // 这个时候也必须要解除引用，因为可能后面会有 xxx.a = 100; 这样的语句出现，所以把它看作左值 lvalue
  left = create_ast_left_node(AST_DEREFERENCE_POINTER, member->primitive_type, left, 0, NULL, member->composite_type);
  return (left);
}

struct ASTNode *convert_prefix_expression_2_ast(int previous_token_precedence) {
  struct ASTNode *tree;
  switch (token_from_file.token) {
    case TOKEN_AMPERSAND:
      // 解析类似于 x= &&&y;
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast(previous_token_precedence);

      if (tree->operation != AST_IDENTIFIER)
        error("& operator must be followed by an identifier");
      if (tree->symbol_table->structural_type == STRUCTURAL_ARRAY)
        error("& operator cannot be performed on an array");

      tree->operation = AST_IDENTIFIER_ADDRESS;
      tree->primitive_type = pointer_to(tree->primitive_type);
      break;
    case TOKEN_MULTIPLY:
      // 解析类似于 x= ***y;
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast(previous_token_precedence);
      tree->rvalue = 1;

      if (!check_pointer_type(tree->primitive_type))
        error("* operator must be followed by an identifier or *");
      // 生成一个父级节点
      tree = create_ast_left_node(
        AST_DEREFERENCE_POINTER,
        value_at(tree->primitive_type),
        tree,
        0,
        NULL,
        tree->composite_type);
      break;
    case TOKEN_MINUS:
      // 解析类似 x = -y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast(previous_token_precedence);

      tree->rvalue = 1;
      if (tree->primitive_type == PRIMITIVE_CHAR)
        tree->primitive_type = PRIMITIVE_INT;

      tree = create_ast_left_node(AST_NEGATE, tree->primitive_type, tree, 0, NULL, tree->composite_type);
      break;
    case TOKEN_INVERT:
      // 解析类似 x = ~y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast(previous_token_precedence);

      tree->rvalue = 1;
      tree = create_ast_left_node(AST_INVERT, tree->primitive_type, tree, 0, NULL, tree->composite_type);
      break;
    case TOKEN_LOGIC_NOT:
      // 解析类似 x = !y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast(previous_token_precedence);

      tree->rvalue = 1;
      tree = create_ast_left_node(AST_LOGIC_NOT, tree->primitive_type, tree, 0, NULL, tree->composite_type);
      break;
    case TOKEN_INCREASE:
      // 解析类似 x = ++y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast(previous_token_precedence);

      if (tree->operation != AST_IDENTIFIER)
        error("++ operator must be followed by an identifier");

      tree = create_ast_left_node(AST_PRE_INCREASE, tree->primitive_type, tree, 0, NULL, tree->composite_type);
      break;
    case TOKEN_DECREASE:
      // 解析类似 x = --y; 这样的表达式
      scan(&token_from_file);
      tree = convert_prefix_expression_2_ast(previous_token_precedence);

      if (tree->operation != AST_IDENTIFIER)
        error("-- operator must be followed by an identifier");

      tree = create_ast_left_node(AST_PRE_DECREASE, tree->primitive_type, tree, 0, NULL, tree->composite_type);
      break;
    default:
      tree = convert_postfix_expression_2_ast(previous_token_precedence);
  }

  return (tree);
}

struct ASTNode *convert_postfix_expression_2_ast(int previous_token_precedence) {
  struct ASTNode *tree = create_ast_node_from_expression(previous_token_precedence);

  // 解析类似的语句时需要注意的问题，即区别是变量名还是函数调用还是数组访问
  //  x = fred + jim;
  //  x = fred(5) + jim;
  //  x = a[12];

  while(1) {
    switch (token_from_file.token) {
      // 如果是左 [，则直接看作是访问数组
      case TOKEN_LEFT_BRACKET:
        tree = convert_array_access_2_ast(tree);
        break;
      // 如果是 '.' 或者 '->' 看作是访问 struct/union
      case TOKEN_DOT:
        tree = convert_member_access_2_ast(0, tree);
        break;
      case TOKEN_ARROW:
        tree = convert_member_access_2_ast(1, tree);
        break;
      case TOKEN_INCREASE:
        if (tree->rvalue)
          error("Cannot ++ on rvalue");
        // 解析类似 x = y++; 语句
        scan(&token_from_file);

        if (tree->operation == AST_POST_INCREASE ||
            tree->operation == AST_POST_DECREASE)
          error("Cannot ++ and/or -- more than once");
        tree->operation = AST_POST_INCREASE;
        break;
      case TOKEN_DECREASE:
        if (tree->rvalue)
          error("Cannot -- on rvalue");
        // 解析类似 x = y--; 语句
        scan(&token_from_file);

        if (tree->operation == AST_POST_INCREASE ||
            tree->operation == AST_POST_DECREASE)
          error("Cannot ++ and/or -- more than once");

        tree->operation = AST_POST_DECREASE;
        break;

      default: return (tree);
    }
  }

  return (NULL);
}
