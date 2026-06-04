#ifndef OLRN_CODEGEN_H
#define OLRN_CODEGEN_H

#include "../ast/ast.h"
#include <stdio.h>

typedef struct {
    FILE *out;
    int   indent;
} Codegen;

void codegen_init(Codegen *cg, FILE *out);
void codegen_emit(Codegen *cg, AstNode *program);

#endif /* OLRN_CODEGEN_H */
