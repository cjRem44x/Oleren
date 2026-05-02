#ifndef EXPR_H
#define EXPR_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum ExprType {
    NONE, COMMENT, LABEL, INSTRUC
} ExprType;

typedef struct Expr {
    const char * token;
    ExprType type;
} Expr;

typedef struct ExprList {
    Expr * dat;
    size_t size;
    size_t cap;

    void   (*resize)(struct ExprList * slf);
    void   (*add)   (struct ExprList * slf, Expr e);
    Expr * (*get)   (struct ExprList * slf, size_t idx);
    void   (*rmv)   (struct ExprList * slf, size_t idx);
    void   (*print) (struct ExprList * slf);
    void   (*_free) (struct ExprList * slf);
} ExprList;

ExprList * init_ExprList(size_t cap);
void resize(struct ExprList * slf);
void add(struct ExprList * slf, Expr e);
Expr * get(struct ExprList * slf, size_t idx);
void rmv(struct ExprList * slf, size_t idx);
void print(struct ExprList * slf);
void _free(struct ExprList * slf);

#endif // !EXPR_H
