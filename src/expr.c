#include "expr.h"
#include <stdio.h>

const char * ExprTypeStr[] = {
    "NONE", "COMMENT", "LABEL", "INSTRUC"
};

ExprList * init_ExprList(size_t cap)
{
    ExprList * p = (ExprList *)malloc(sizeof(ExprList));
    if (p == NULL) perror("NULL ExprList");

    p->size = 0;
    p->cap = cap > 0 ? cap : 1;
    p->dat = (Expr *)malloc(p->cap * sizeof(Expr));
    if (p->dat == NULL) perror("NULL dat");

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
        Expr * tmp = realloc(slf->dat, slf->cap * sizeof(Expr));
        if (tmp == NULL) {perror("realloc failure!"); return;}
        slf->dat = tmp;
    }
}

void add(struct ExprList * slf, Expr e)
{
    slf->resize(slf);
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


void print(struct ExprList * slf)
{
    for (size_t i=0; i<slf->size; i++)
    {
        printf("TOKEN: %s\n", slf->dat[i].token);
        printf("TYPE:  %s\n", ExprTypeStr[slf->dat[i].type]);
    }
}

void _free(struct ExprList * slf)
{
    //for (size_t i=0; i<slf->size; i++){}
    free(slf->dat);
    free(slf);
}
