#ifndef __GENERATOR_H__
#define __GENERATOR_H__

int interpret_ast_with_register(struct ASTNode *node, int register_index);
void generate_code(struct ASTNode *node);
void generate_preamble_code();
void generate_postamble_code();
void generate_clearable_registers();
void generate_printable_code(int register_index);
void generate_global_symbol_table_code(char *symbol_string);

#endif
