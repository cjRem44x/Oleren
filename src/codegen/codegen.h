#ifndef OLRN_CODEGEN_H
#define OLRN_CODEGEN_H

#include "../ast/ast.h"
#include <stdio.h>

#define MAX_TYPE_VARS 32

typedef struct {
    FILE *out;
    int   indent;
    int   in_template;                  /* inside a function with any params */
    const char *type_vars[MAX_TYPE_VARS]; /* names bound via T :: @type(x) */
    int   type_var_count;
} Codegen;

void codegen_init(Codegen *cg, FILE *out);
void codegen_emit(Codegen *cg, AstNode *program);

#endif /* OLRN_CODEGEN_H */
