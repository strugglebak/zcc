#include "definations.h"
#include "generator_core.h"
#include "helper.h"

// 比较俩基本类型是否兼容
// 返回 1 为兼容，0 为不兼容

// 如果 only_right 为 true，则表示只能从 left 扩展到 right
// 比如对于 char i = 1; int j = i; 这种情况
// 如果此时 stop_type_to_widen 是 true, 那么将不会出现强制转换
int check_type_compatible(
  int *left_primitive_type,
  int *right_primitive_type,
  int stop_type_to_widen
) {
  int left_primitive_type_size, right_primitive_type_size;

  // 检查是否一致
  if (*left_primitive_type == *right_primitive_type) {
    *left_primitive_type = *right_primitive_type = 0;
    return 1;
  }

  // 用 size 来检查是否一致
  left_primitive_type_size = register_get_primitive_type_size(*left_primitive_type);
  right_primitive_type_size = register_get_primitive_type_size(*right_primitive_type);

  if (left_primitive_type_size == 0 ||
      right_primitive_type_size == 0)
      return 0;


  // 检查不一致时的处理
  if (left_primitive_type_size < right_primitive_type_size) {
    *left_primitive_type = AST_WIDEN;
    *right_primitive_type = 0;
    return 1;
  }
  if (left_primitive_type_size > right_primitive_type_size) {
    if (stop_type_to_widen) return 0;
    *left_primitive_type = 0;
    *right_primitive_type = AST_WIDEN;
    return 1;
  }

  // 剩下的都是兼容的
  *left_primitive_type = *right_primitive_type = 0;
  return 1;
}

int pointer_to(int primitive_type) {
  int new_type;
  switch (primitive_type) {
    case PRIMITIVE_VOID: new_type = PRIMITIVE_VOID_POINTER; break;
    case PRIMITIVE_CHAR: new_type = PRIMITIVE_CHAR_POINTER; break;
    case PRIMITIVE_INT: new_type = PRIMITIVE_INT_POINTER; break;
    case PRIMITIVE_LONG: new_type = PRIMITIVE_LONG_POINTER; break;
    default:
      error_with_digital("Unrecognised in pointer_to: primitive type", primitive_type);
  }
  return new_type;
}

int value_at(int primitive_type) {
  int new_type;
  switch (primitive_type) {
    case PRIMITIVE_VOID_POINTER: new_type = PRIMITIVE_VOID; break;
    case PRIMITIVE_CHAR_POINTER: new_type = PRIMITIVE_CHAR; break;
    case PRIMITIVE_INT_POINTER: new_type = PRIMITIVE_INT; break;
    case PRIMITIVE_LONG_POINTER: new_type = PRIMITIVE_LONG; break;
    default:
      error_with_digital("Unrecognised in value_at: primitive type", primitive_type);
  }
  return new_type;
}
