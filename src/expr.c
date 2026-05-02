#include "expr.h"

ExprList * init_ExprList(size_t cap)
{
    ExprList * p = (ExprList *)malloc(sizeof(ExprList));
    if (p == NULL) perror("NULL ExprList");

    p->size = 0;
    p->cap = cap;
    p->dat = (Expr *)malloc(p->cap * sizeof(Expr));
    return p;
}

void resize(struct ExprList * slf);
void add(struct ExprList * slf, Expr e);
Expr * get(struct ExprList * slf, size_t idx);
void rmv(struct ExprList * slf, size_t idx);
void _free(struct ExprList * slf);

