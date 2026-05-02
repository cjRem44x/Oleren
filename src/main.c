#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "expr.h"

#define BUF_SIZE 1024

#define INSTRUC_CHAR '|'
#define LABEL_CHAR   '@'
#define COMMENT_CHAR '#'

ExprList * exprs;

void proc_expr(Expr tmp)
{
    printf("TOKEN: %s\n", tmp.token);
    exprs->add(exprs, tmp);
}

int main(int argc, char ** argv)
{
    exprs = init_ExprList(1024);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    FILE * f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    char buf[BUF_SIZE];
    size_t idx = 0;

    int c;
    while ((c = fgetc(f)) != EOF)
    {
        if (c == '\0') continue;  // skip UTF-16 null bytes
        if (isspace(c)) continue;

        switch (c) {
            case INSTRUC_CHAR:
            case LABEL_CHAR:
            case COMMENT_CHAR: {
                buf[idx] = '\0';
                if (idx > 0) {
                    ExprType type = (c == INSTRUC_CHAR) ? INSTRUC
                                    : (c == LABEL_CHAR)   ? LABEL
                                    : COMMENT;
                    Expr tmp = {buf, type};
                    proc_expr(tmp);
                    idx = 0;
                }
                break;
            }
            default:
                if (idx < BUF_SIZE - 1) {
                    buf[idx++] = (char)c;
                } else {
                    fprintf(stderr, "Token too LONG\n");
                    idx = 0;
                }
        }
    }
    if (idx > 0) {
        buf[idx] = '\0';
        Expr tmp = {buf, INSTRUC};
        proc_expr(tmp);
        idx = 0;
    }
    fclose(f);

    exprs->_free(exprs);
    return 0;
}
