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
    NODE_ASSIGN,          /* lhs op rhs  (= += -= *= /= %=)  */
    NODE_WHILE,           /* while cond { }                  */
    NODE_LOOP,            /* loop init, cond, step { }       */
    NODE_FOR_RANGE,       /* for lo..hi { }                  */
    NODE_FOR_EACH,        /* for e => iter { }               */
    NODE_EXTERN_FN,       /* extern fn declaration (no body) */
    NODE_IMPORT_DECL,     /* one entry in an import block */
    NODE_CALL_EXPR,       /* callee-expression call       */
    NODE_VAR_DECL,        /* single variable declaration  */
    NODE_VAR_DECL_GROUP,  /* mut/imu T: a=v, b=v, ...     */
    NODE_IF,          /* if/elif/else           */
    NODE_WHEN,        /* when subject { arms }  */
    NODE_WHEN_ARM,    /* pattern => body        */
    NODE_BINARY,      /* left op right          */
    NODE_UNARY,       /* op operand (prefix)    */
    NODE_FIELD,       /* target.name            */
    NODE_FIELD_PTR,   /* target->name           */
    NODE_SUBSCRIPT,   /* target[index]          */
    NODE_DEREF,       /* target.* (Oleren deref)*/
    NODE_STR_LIT,
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_CHAR_LIT,
    NODE_BOOL_LIT,
    NODE_IDENT,
    NODE_DEFER,         /* defer expr | defer { }           */
    NODE_ARRAY_LIT,     /* {e0, e1, ...}                    */
    NODE_STRUCT_LIT,    /* Foo{.f=v,...} or .{.f=v,...}     */
    NODE_FIELD_INIT,    /* .name = val, within struct lit    */
    NODE_STRUCT_DECL,   /* struct Foo { fields }             */
    NODE_ENUM_DECL,     /* enum X { ... } or enum X => T {} */
    NODE_ENUM_VARIANT,  /* single enum variant               */
    NODE_UNN_DECL,      /* unn X { ... }                    */
    NODE_TYPE_ALIAS,    /* type X = T                       */
    /* error handling */
    NODE_ERR_DECL,      /* err Name { A, B, C }             */
    NODE_ERR_LIT,       /* err.NAME — inline error literal  */
    NODE_TRY_EXPR,      /* try expr                         */
    NODE_CATCH_EXPR,    /* expr catch fallback|expr catch |e| {} */
    NODE_ERRDEFER,      /* errdefer expr                    */
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
        struct { NodeList imports; NodeList decls; } program;

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
            int   is_ptr;    /* * raw pointer  */
            int   is_smart;  /* ^ smart pointer */
            int   is_arr;    /* [] or [N] array */
            int   arr_size;  /* 0 = dynamic, N = fixed [N] */
            int   is_imu;    /* immutable elems */
            int   is_result; /* !T error union  */
            char *err_set;   /* ErrSet!T set name; NULL = generic !T */
        } type_ref;

        /* NODE_BUILTIN_CALL / NODE_CALL */
        struct { char *name; NodeList args; } call;

        /* NODE_RET */
        struct { AstNode *value; /* NULL for bare ret */ } ret;

        /* NODE_ASSIGN — lhs op= rhs */
        struct { int op; AstNode *lhs; AstNode *rhs; } assign;

        /* NODE_WHILE */
        struct { AstNode *cond; AstNode *body; } while_loop;

        /* NODE_LOOP — loop init, cond, step { } */
        struct { AstNode *init; AstNode *cond; AstNode *step; AstNode *body; } loop_stmt;

        /* NODE_FOR_RANGE — for lo..hi { } */
        struct { AstNode *lo; AstNode *hi; int inclusive; AstNode *body; } for_range;

        /* NODE_FOR_EACH — for e => iter or for e, i => iter */
        struct {
            char    *elem;  /* NULL or "_" for ignored elem */
            char    *idx;   /* NULL if no index var */
            AstNode *iter;
            AstNode *body;
        } for_each;

        /* NODE_EXTERN_FN — C function declaration, no body */
        struct {
            char    *name;
            NodeList params;
            AstNode *ret_type; /* NULL = void */
            int      is_variadic;
        } extern_fn;

        /* NODE_IMPORT_DECL — one alias = source entry, or a top-level
           module bind (io :: @std.io) */
        struct {
            char *alias;
            char *source;  /* file path or lib name ("std", "pkg", ...) */
            char *module;  /* submodule for @std.X binds; NULL = whole lib */
            int   is_lib;  /* 0 = local file, 1 = @std / @std.X */
        } import_decl;

        /* NODE_CALL_EXPR — callee is an arbitrary expression */
        struct { AstNode *callee; NodeList args; } call_expr;

        /* NODE_VAR_DECL — type_ref NULL = auto-infer; init NULL = undef */
        struct {
            char    *name;
            int      is_imu;
            AstNode *type_ref;
            AstNode *init;
        } var_decl;

        /* NODE_VAR_DECL_GROUP — mut/imu T: a=v, b=v, ... */
        struct {
            int      is_imu;
            AstNode *type_ref;
            NodeList entries; /* NODE_VAR_DECL items (type_ref is NULL in each) */
        } var_decl_group;

        /* NODE_IF — else_block is NULL, NODE_BLOCK, or NODE_IF (elif chain) */
        struct {
            AstNode *cond;
            AstNode *then_block;
            AstNode *else_block;
        } if_expr;

        /* NODE_WHEN */
        struct { AstNode *subject; NodeList arms; } when_expr;

        /* NODE_WHEN_ARM — pattern NULL means default (_) */
        struct { AstNode *pattern; AstNode *body; } when_arm;

        /* NODE_BINARY */
        struct { int op; AstNode *left; AstNode *right; } binary;

        /* NODE_UNARY */
        struct { int op; AstNode *operand; } unary;

        /* NODE_FIELD / NODE_FIELD_PTR */
        struct { AstNode *target; char *name; } field;

        /* NODE_SUBSCRIPT */
        struct { AstNode *target; AstNode *index; } subscript;

        /* NODE_DEREF */
        struct { AstNode *target; } deref;

        /* literals */
        struct { char      *value; } str_lit;
        struct { long long  value; } int_lit;
        struct { double     value; } float_lit;
        struct { char       value; } char_lit;
        struct { int        value; } bool_lit;

        /* NODE_IDENT */
        struct { char *name; } ident;

        /* NODE_DEFER — expr is a single expression or a NODE_BLOCK */
        struct { AstNode *expr; } defer_stmt;

        /* NODE_ARRAY_LIT — {e0, e1, ...} */
        struct { NodeList elems; } array_lit;

        /* NODE_STRUCT_LIT — Foo{.f=v,...} or .{.f=v,...} (type_name NULL = anon) */
        struct { char *type_name; NodeList fields; } struct_lit;

        /* NODE_FIELD_INIT — .name = val entry within a struct literal */
        struct { char *name; AstNode *value; } field_init;

        /* NODE_STRUCT_DECL — struct Foo { field: type, ... } */
        struct { char *name; NodeList fields; } struct_decl;

        /* NODE_ENUM_DECL — plain or typed (base_type NULL = plain) */
        struct {
            char    *name;
            AstNode *base_type;  /* NULL = plain enum class, else => T typed */
            NodeList variants;
        } enum_decl;

        /* NODE_ENUM_VARIANT — name with optional value */
        struct { char *name; AstNode *value; /* NULL for plain */ } enum_variant;

        /* NODE_UNN_DECL — union Foo { a: T, b: U, ... } */
        struct { char *name; int is_tagged; NodeList fields; } unn_decl;

        /* NODE_TYPE_ALIAS — type Name = T */
        struct { char *name; AstNode *target; } type_alias;

        /* NODE_ERR_DECL — err Name { A, B, C } */
        struct { char *name; NodeList variants; } err_decl;

        /* NODE_ERR_LIT — err.NAME (anonymous) or used as error value */
        struct { char *set_name; char *variant_name; } err_lit;

        /* NODE_TRY_EXPR — try expr */
        struct { AstNode *expr; } try_expr;

        /* NODE_CATCH_EXPR — expr catch fallback  or  expr catch |e| { body } */
        struct {
            AstNode *expr;
            AstNode *fallback; /* NULL for block form */
            char    *err_var;  /* NULL for value form */
            AstNode *body;     /* NULL for value form */
        } catch_expr;

        /* NODE_ERRDEFER — errdefer expr */
        struct { AstNode *expr; } errdefer_stmt;
    };
};

AstNode *ast_node_new(NodeKind kind, int line);
void     node_list_push(NodeList *list, AstNode *node);
void     ast_free(AstNode *node);
void     ast_print(AstNode *node, int indent);

#endif /* OLRN_AST_H */
