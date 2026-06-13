#ifndef OLRN_PARSER_H
#define OLRN_PARSER_H

#include "../lexer/lexer.h"
#include "../ast/ast.h"

typedef struct {
    Lexer *lexer;
    Token  cur;
    Token  peek;
    int    had_error;
    /* @map variable names — for for-each disambiguation */
    char   map_vars[64][64];
    int    map_var_count;
} Parser;

void     parser_init(Parser *p, Lexer *l);
AstNode *parser_parse_program(Parser *p);

#endif /* OLRN_PARSER_H */
