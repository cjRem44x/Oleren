#ifndef OLRN_CODEGEN_H
#define OLRN_CODEGEN_H

#include "../ast/ast.h"
#include <stdio.h>

#define MAX_TYPE_VARS    32
#define MAX_IMPORT_ALIAS 32
#define MAX_ENUM_NAMES   64
#define MAX_ERR_NAMES    64

typedef struct {
    FILE *out;
    int   indent;
    int   in_template;                        /* inside a function with any params */
    const char *type_vars[MAX_TYPE_VARS];     /* names bound via T :: @type(x) */
    int   type_var_count;
    const char *import_aliases[MAX_IMPORT_ALIAS]; /* import alias names */
    const char *import_modules[MAX_IMPORT_ALIAS]; /* submodule per alias, NULL = whole lib */
    int   import_alias_count;
    int   has_stdlib;    /* @std was imported */
    int   has_malkur;    /* @std.malkur was bound */
    int   has_pelentar;  /* @std.pelentar was bound */
    int   defer_counter; /* unique ID for each deferred guard in a function */
    const char *enum_names[MAX_ENUM_NAMES]; /* known enum type names for :: access */
    int   enum_count;
    /* error handling state */
    const char *err_names[MAX_ERR_NAMES]; /* known error set names */
    int   err_count;
    int   in_err_fn;        /* 1 if current function returns !T */
    char  err_ret_cpp[128]; /* C++ success type for current !T fn (e.g. "int32_t") */
    int   has_errdefer;     /* 1 if current function has errdefer stmts */
    int   try_counter;      /* unique ID for _r_N try/catch temporaries */
} Codegen;

void codegen_init(Codegen *cg, FILE *out);
void codegen_emit(Codegen *cg, AstNode *program);

#endif /* OLRN_CODEGEN_H */
