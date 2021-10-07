#ifndef __TYPES_H__
#define __TYPES_H__

int pointer_to(int primitive_type);
int value_at(int primitive_type);
int check_int_type(int primitive_type);
int check_pointer_type(int primitive_type);
int get_primitive_type_size(
  int primitive_type,
  struct SymbolTable *composite_type
);
struct ASTNode *modify_type(
  struct ASTNode *tree,
  int right_primitive_type,
  int operation
);

#endif
