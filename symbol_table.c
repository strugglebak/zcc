#include <string.h>
#include "helper.h"
#include "data.h"
#include "generator.h"

// symbol_table
// ^
// |
// 0xxxx......................................xxxxxxxxxxxxSYMBOL_TABLE_ENTRIES_NUMBER-1
//      ^                                    ^
//      |                                    |
// 全局变量位置                             局部变量位置

/**
 * 创建新的全局变量
*/
static int new_global_symbol_string() {
  int index = 0;
  if ((index = global_symbol_table_index++) >= local_symbol_table_index) {
    error("Too many global symbols");
  }
  return index;
}

/**
 * 创建新的局部变量
*/
static int new_local_symbol_string() {
  int index = 0;
  if ((index = local_symbol_table_index--) <= global_symbol_table_index) {
    error("Too many local symbols");
  }
  return index;
}

/**
 * 更新 symbol_table
*/
static void update_symbol_table(
  int index,
  char *name,
  int primitive_type,
  int structural_type,
  int end_label,
  int size,
  int storage_class,
  int position
) {
  if (index < 0 || index >= SYMBOL_TABLE_ENTRIES_NUMBER)
    error("Invalid symbol slot number in update_symbol_table()");
  symbol_table[index].name = strdup(name);
  symbol_table[index].primitive_type = primitive_type;
  symbol_table[index].structural_type = structural_type;
  symbol_table[index].end_label = end_label;
  symbol_table[index].size = size;
  symbol_table[index].storage_class = storage_class;
  symbol_table[index].position = position;
}

/**
 * 返回这个 全局 symbol_string 在 symbol_table 中的位置，如果没找到则返回 -1
*/
int find_global_symbol_table_index(char *symbol_string) {
  for (int i = 0; i < global_symbol_table_index; i++) {
    // 如果首字母相同, 并且后续的字符串都相同
    if (*symbol_string == *(symbol_table[i].name)
      && !strcmp(symbol_string, symbol_table[i].name)) {
        return i;
    }
  }

  return -1;
}

/**
 * 返回这个 局部 symbol_string 在 symbol_table 中的位置，如果没找到则返回 -1
*/
int find_local_symbol_table_index(char *symbol_string) {
  for (int i = local_symbol_table_index+1; i < SYMBOL_TABLE_ENTRIES_NUMBER; i++) {
    // 如果首字母相同, 并且后续的字符串都相同
    if (*symbol_string == *(symbol_table[i].name)
      && !strcmp(symbol_string, symbol_table[i].name)) {
        return i;
    }
  }

  return -1;
}

/**
 * 将一个 全局 symbol_string 插入到 global_symbol_string 中，返回这个被插入的下标
*/
int add_global_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int end_label,
  int size
) {
  int index = 0;
  if ((index = find_global_symbol_table_index(symbol_string)) != -1) {
    return index;
  }

  index = new_global_symbol_string();
  // 将字符串复制一份放入到内存中，并返回这个内存的地址
  // 初始化
  update_symbol_table(
    index,
    symbol_string,
    primitive_type,
    structural_type,
    end_label,
    size,
    CENTRAL_GLOBAL,
    0
  );
  generate_global_symbol(index);
  return index;
}

/**
 * 将一个 局部 symbol_string 插入到 global_symbol_string 中，返回这个被插入的下标
*/
int add_local_symbol(
  char *symbol_string,
  int primitive_type,
  int structural_type,
  int end_label,
  int size
) {
  int index = 0;
  int position = 0;
  if ((index = find_local_symbol_table_index(symbol_string)) != -1) {
    return index;
  }

  index = new_local_symbol_string();
  position = generate_get_local_offset(primitive_type, 0);
  // 将字符串复制一份放入到内存中，并返回这个内存的地址
  // 初始化
  update_symbol_table(
    index,
    symbol_string,
    primitive_type,
    structural_type,
    end_label,
    size,
    CENTRAL_LOCAL,
    position
  );
  return index;
}

int find_symbol(char *string) {
  // 优先找 local
  int index = find_local_symbol_table_index(string);
  if (index < 0)
    // local 找不到再去找全局
    index = find_global_symbol_table_index(string);
  return index;
}
