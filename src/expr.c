#include "expr.h"

ExprList * init_ExprList(size_t cap)
{
    ExprList * p = (ExprList *)malloc(sizeof(ExprList));
    if (p == NULL) perror("NULL ExprList");

    p->size = 0;
    p->cap = cap;
    p->dat = (Expr *)malloc(p->cap * sizeof(Expr));
    
    p->resize=resize;
    p->add=add;
    p->get=get;
    p->rmv=rmv;
    p->_free=_free;
    return p;
}

void resize(struct ExprList * slf)
{
    if (slf->size >= slf->cap) {
        slf->cap *= 2;
        slf->dat = realloc(slf->dat, slf->cap * sizeof(Expr));
    }
}

void add(struct ExprList * slf, Expr e)
{
    resize(slf);
    slf->dat[slf->size++] = e;
}

Expr * get(struct ExprList * slf, size_t idx)
{
    if (idx >= slf->size) return NULL;

    return &slf->dat[idx];
}

void rmv(struct ExprList * slf, size_t idx)
{
    if (idx >= slf->size) return;

    for (size_t i=idx; i<slf->size-1; i++)
    {
        slf->dat[i] = slf->dat[i+1];
    }

    slf->size--;
}

void _free(struct ExprList * slf)
{
    //for (size_t i=0; i<slf->size; i++){}
    free(slf->dat);
}
