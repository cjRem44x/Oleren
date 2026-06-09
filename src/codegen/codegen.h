#ifndef OLRN_CODEGEN_H
#define OLRN_CODEGEN_H

#include "../ast/ast.h"
#include <stdio.h>

#define MAX_TYPE_VARS    32
#define MAX_IMPORT_ALIAS 32
#define MAX_ENUM_NAMES   64

typedef struct {
    FILE *out;
    int   indent;
    int   in_template;                        /* inside a function with any params */
    const char *type_vars[MAX_TYPE_VARS];     /* names bound via T :: @type(x) */
    int   type_var_count;
    const char *import_aliases[MAX_IMPORT_ALIAS]; /* import alias names */
    int   import_alias_count;
    int   has_stdlib;    /* @libs.std was imported */
    int   defer_counter; /* unique ID for each deferred guard in a function */
    const char *enum_names[MAX_ENUM_NAMES]; /* known enum type names for :: access */
    int   enum_count;
} Codegen;

void codegen_init(Codegen *cg, FILE *out);
void codegen_emit(Codegen *cg, AstNode *program);

#endif /* OLRN_CODEGEN_H */
