#include <stdio.h>
#include "definations.h"
#include "generator.h"
#include "helper.h"
#include "types.h"
#include "ast.h"

int check_int_type(int primitive_type) {
  return ((primitive_type & 0xf) == 0);
}

int check_pointer_type(int primitive_type) {
  return ((primitive_type & 0xf) != 0);
}

// 修改一个 ast node 的 type 类型，以便与给定的类型兼容
// 对于指针来说，如果是一个二元运算(+ 或者 -)，那么 ast 的 operation 就是非 0
struct ASTNode *modify_type(
  struct ASTNode *tree,
  int right_primitive_type,
  int operation) {
    int left_primitive_size, right_primitive_size;
    int left_primitive_type = tree->primitive_type;

    // 左右改变类似于如下的计算
    // x= *ptr;	        // OK
    // x= *(ptr + 2);   // 取从 ptr 指向的地址 + 两个整数的地方的值
    // x= *(ptr * 4);   // 无意义
    // x= *(ptr / 13);  // 无意义

    // 比较 char/int/long
    if (check_int_type(left_primitive_type) && check_int_type(right_primitive_type)) {
      if (left_primitive_type == right_primitive_type) return tree;

      left_primitive_size = generate_get_primitive_type_size(left_primitive_type);
      right_primitive_size = generate_get_primitive_type_size(right_primitive_type);

      if (left_primitive_size > right_primitive_size) return NULL;
      else if (left_primitive_size < right_primitive_size)
        return create_ast_left_node(AST_WIDEN, right_primitive_type, tree, 0, NULL);
    }

    // 比较 char/int/long 指针
    if (check_pointer_type(left_primitive_type)) {
      if (!operation && left_primitive_type == right_primitive_type) return tree;
    }

    // 指针的地址只允许加和减
    if (operation == AST_PLUS || operation == AST_MINUS) {
      if (check_int_type(left_primitive_type) &&
          check_pointer_type(right_primitive_type)) {
        right_primitive_size =
          generate_get_primitive_type_size(value_at(right_primitive_type));
        // size 比 int 大的
        // 比如 int x = *((long *) y + 2)
        // 强制转换
        if (right_primitive_size > 1)
          return create_ast_left_node(
            AST_SCALE,
            right_primitive_type,
            tree,
            right_primitive_size,
            NULL);
        // 如果 size 就是 int，那么就返回这颗树
        return tree;
      }
    }

    return NULL;
}


int pointer_to(int primitive_type) {
  if ((primitive_type & 0xf) == 0xf)
    error_with_digital("Unrecognised in pointer_to: primitive type", primitive_type);
  return primitive_type + 1;
}

int value_at(int primitive_type) {
  if ((primitive_type & 0xf) == 0x0)
    error_with_digital("Unrecognised in value_at: primitive type", primitive_type);
  return primitive_type - 1;
}
