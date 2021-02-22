
#ifndef __DATA_H__
#define __DATA_H__

#include <stdio.h>
#include "definations.h"

#ifndef extern_
  #define extern_ extern
#endif

#define GET_ARRAY_LENGTH(x)(sizeof(x) / sizeof((x)[0]))

extern_ int line;
extern_ int putback_buffer;
extern_ FILE *input_file;
extern_ FILE *output_file;
extern_ struct Token token_from_file;

#endif
