#ifndef OLRN_MODULE_RESOLVER_H
#define OLRN_MODULE_RESOLVER_H

#include "../ast/ast.h"

AstNode *resolver_parse_file(const char *path, char **src_out);
int resolver_merge_imports(AstNode *program, const char *host_path,
                           char **extra_srcs, int *extra_count, int max);

#endif /* OLRN_MODULE_RESOLVER_H */
