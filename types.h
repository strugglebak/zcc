#ifndef __TYPES_H__
#define __TYPES_H__

int check_type_compatible(
  int *left_primitive_type,
  int *right_primitive_type,
  int stop_type_to_widen
);
int pointer_to(int primitive_type);
int value_at(int primitive_type);

#endif
