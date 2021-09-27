#ifndef __DECLARATION_H__
#define __DECLARATION_H__


int convert_token_2_primitive_type();
void parse_var_declaration_statement(int primitive_type, int storage_class);
struct ASTNode *parse_function_declaration_statement(int primitive_type);
void parse_global_declaration_statement();

#endif
