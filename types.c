#include "definations.h"

// 比较俩基本类型是否兼容
// 返回 1 为兼容，0 为不兼容

// 如果 only_right 为 true，则表示只能从 left 扩展到 right
// 比如对于 char i = 1; int j = i; 这种情况
// 如果此时 only_right 是 true, 那么将不会出现强制转换
int check_type_compatible(int *left, int *right, int only_right) {
  // 1. 检查 void
  if ((*left == PRIMITIVE_VOID) || (*right == PRIMITIVE_VOID)) return 0;

  // 2. 检查是否一致
  if (*left == *right) {
    *left = *right = 0;
    return 1;
  }

  // 3. 检查不一致时的处理
  if ((*left == PRIMITIVE_CHAR) && (*right == PRIMITIVE_INT)) {
    *left = AST_WIDEN;
    *right = 0;
    return 1;
  }
  if ((*left == PRIMITIVE_INT) && (*right == PRIMITIVE_CHAR)) {
    if (only_right) return 0;
    *left = 0;
    *right = AST_WIDEN;
    return 1;
  }

  // 4. 剩下的都是兼容的
  *left = *right = 0;
  return 1;
}
