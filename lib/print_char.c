#include <stdio.h>

void print_char(long x) {
  putc((char)(x & 0x7f), stdout);
}
