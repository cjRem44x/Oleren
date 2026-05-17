#include "olnas.h"
#include "expr.h"

ExprList * exprs;

void proc_expr(Expr tmp)
{
    printf("TOKEN: %s\n", tmp.token);
    exprs->add(exprs, tmp);
}

void begin_olnas(char * filePath)
{
    exprs = init_ExprList(BUF_SIZE);

    FILE * f = fopen(filePath, "r"); // file path
    if (!f) {
        perror("fopen");
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

    printf("TOKEN SIZE = %lu\n", exprs->size);

    exprs->_free(exprs);
}
