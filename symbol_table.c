#include <string.h>
#include "helper.h"
#include "data.h"

static int global_symbol_table_index = 0;

/**
 * 在 global_symbol_table 这个数组中找到一个新的下标，并返回这个下标
*/
static int new_global_symbol_string() {
  int index = 0;
  if ((index = global_symbol_table_index++) >= SYMBOL_TABLE_ENTRIES_NUMBER) {
    error("Too many global symbols");
  }
  return index;
}

/**
 * 返回这个 symbol_string 在 global_symbol_table 中的位置，如果没找到则返回 -1
*/
int find_global_symbol_table_index(char *symbol_string) {
  for (int i = 0; i < global_symbol_table_index; i++) {
    // 如果首字母相同, 并且后续的字符串都相同
    if (*symbol_string == *(global_symbol_table[i].name)
      && !strcmp(symbol_string, global_symbol_table[i].name)) {
        return i;
    }
  }

  return -1;
}

/**
 * 将一个 symbol_string 插入到 global_symbol_string 中，返回这个被插入的下标
*/
int add_global_symbol(char *symbol_string) {
  int index = 0;
  if ((index = find_global_symbol_table_index(symbol_string)) != 1) {
    return index;
  }

  index = new_global_symbol_string();
  // 将字符串复制一份放入到内存中，并返回这个内存的地址
  global_symbol_table[index].name = strdup(symbol_string);
  return index;
}
