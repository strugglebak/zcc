#include <stdio.h>
#include "definations.h"

#ifndef extern_
  #define extern_ extern
#endif

extern_ int line;
extern_ int putback_buffer;
extern_ FILE *input_file;
extern_ struct Token token_from_file;
