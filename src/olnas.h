#ifndef OLNAS_H
#define OLNAS_H

#define BUF_SIZE 1024

#define INSTRUC_CHAR '|'
#define LABEL_CHAR   '@'
#define COMMENT_CHAR '#'

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "expr.h"

void proc_expr(Expr tmp);
void begin_olnas(char * filePath);

#endif
