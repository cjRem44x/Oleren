#ifndef OLRN_AST_H
#define OLRN_AST_H

typedef enum {
    NODE_PROGRAM,
    NODE_FN_DECL,
    NODE_PARAM,
    NODE_BLOCK,
    NODE_TYPE_REF,
    NODE_BUILTIN_CALL,
    NODE_CALL,
    NODE_RET,
    NODE_STR_LIT,
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_CHAR_LIT,
    NODE_BOOL_LIT,
    NODE_IDENT,
} NodeKind;

typedef struct AstNode AstNode;

typedef struct {
    AstNode **items;
    int       count;
    int       cap;
} NodeList;

struct AstNode {
    NodeKind kind;
    int      line;
    union {
        /* NODE_PROGRAM */
        struct { NodeList decls; } program;

        /* NODE_FN_DECL */
        struct {
            char    *name;
            NodeList params;
            AstNode *ret_type; /* NULL means void */
            AstNode *body;
        } fn_decl;

        /* NODE_PARAM */
        struct { char *name; AstNode *type; } param;

        /* NODE_BLOCK */
        struct { NodeList stmts; } block;

        /* NODE_TYPE_REF */
        struct {
            char *name;
            int   is_ptr;   /* * raw pointer  */
            int   is_smart; /* ^ smart pointer */
            int   is_arr;   /* [] array        */
            int   is_imu;   /* immutable elems */
        } type_ref;

        /* NODE_BUILTIN_CALL / NODE_CALL */
        struct { char *name; NodeList args; } call;

        /* NODE_RET */
        struct { AstNode *value; /* NULL for bare ret */ } ret;

        /* literals */
        struct { char      *value; } str_lit;
        struct { long long  value; } int_lit;
        struct { double     value; } float_lit;
        struct { char       value; } char_lit;
        struct { int        value; } bool_lit;

        /* NODE_IDENT */
        struct { char *name; } ident;
    };
};

AstNode *ast_node_new(NodeKind kind, int line);
void     node_list_push(NodeList *list, AstNode *node);
void     ast_free(AstNode *node);
void     ast_print(AstNode *node, int indent);

#endif /* OLRN_AST_H */
