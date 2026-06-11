#ifndef OLRN_CHECK_H
#define OLRN_CHECK_H

#include "../ast/ast.h"

/* Semantic checks run after parsing/import-merge, before codegen.
   Currently: ErrSet!T enforcement — returned errors must belong to the
   declared set (and name a real variant), and try'd calls may not
   propagate a different named set. Returns the number of errors found
   (0 = ok); diagnostics go to stderr. */
int check_program(AstNode *program);

#endif /* OLRN_CHECK_H */
