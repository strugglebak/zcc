#ifndef __STATEMENT_H__
#define __STATEMENT_H__

struct ASTNode *parse_if_statement();
struct ASTNode *parse_while_statement();
struct ASTNode *parse_for_statement();
struct ASTNode *parse_compound_statement();

#endif
