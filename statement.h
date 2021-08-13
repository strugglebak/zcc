#ifndef __STATEMENT_H__
#define __STATEMENT_H__

struct ASTNode *parse_print_statement();
void parse_var_declaration_statement();
struct ASTNode *parse_assignment_statement();
struct ASTNode *parse_if_statement();
struct ASTNode *parse_while_statement();
struct ASTNode *parse_for_statement();
struct ASTNode *parse_compound_statement();

struct ASTNode *parse_function_declaration_statement();

#endif
