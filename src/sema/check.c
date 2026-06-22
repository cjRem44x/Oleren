#include "check.h"
#include "../lexer/lexer.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAX_ERR_SETS   64
#define MAX_FNS        1024
#define MAX_IMPORTS    64
#define MAX_SYMS       1024
#define MAX_SCOPES     128
#define MAX_STRUCT_REG 128
#define MAX_FIELD_REG  64

/* -----------------------------------------------------------------------
   Simple type representation
   ----------------------------------------------------------------------- */

typedef enum {
    TY_UNKNOWN = 0,  /* can't determine — skip checks involving this */
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_I8, TY_I16, TY_I32, TY_I64,
    TY_U8, TY_U16, TY_U32, TY_U64, TY_USIZE,
    TY_F32, TY_F64,
    TY_STR,
    TY_PTR,    /* raw pointer *T  */
    TY_SMART,  /* smart pointer ^T */
    TY_NAMED,  /* struct / enum / union / err set — name stored separately */
    TY_LIST,   /* @ls(T)      — name = elem type name */
    TY_MAP,    /* @map(K,V)   — name = value type name */
    TY_SET,    /* @set(T)     — name = elem type name */
} TyKind;

typedef struct { TyKind kind; const char *name; } OlrnType;

typedef struct { const char *fname; OlrnType ftype; } RegField;
typedef struct {
    const char *sname;
    RegField    fields[MAX_FIELD_REG];
    int         field_count;
} RegStruct;

#define OTY(k)       ((OlrnType){(k), NULL})
#define OTY_NAMED(n) ((OlrnType){TY_NAMED, (n)})
#define OTY_UNKNOWN  OTY(TY_UNKNOWN)

static const char *ty_name(TyKind k)
{
    switch (k) {
        case TY_UNKNOWN: return "unknown";
        case TY_VOID:    return "void";
        case TY_BOOL:    return "bool";
        case TY_CHAR:    return "char";
        case TY_I8:      return "i8";
        case TY_I16:     return "i16";
        case TY_I32:     return "i32";
        case TY_I64:     return "i64";
        case TY_U8:      return "u8";
        case TY_U16:     return "u16";
        case TY_U32:     return "u32";
        case TY_U64:     return "u64";
        case TY_USIZE:   return "usize";
        case TY_F32:     return "f32";
        case TY_F64:     return "f64";
        case TY_STR:     return "str";
        case TY_PTR:     return "*T";
        case TY_SMART:   return "^T";
        case TY_NAMED:   return "<named>";
        case TY_LIST:    return "@ls";
        case TY_MAP:     return "@map";
        case TY_SET:     return "@set";
        default:         return "?";
    }
}

static int ty_is_int(TyKind k)
{
    return k == TY_I8  || k == TY_I16 || k == TY_I32  || k == TY_I64  ||
           k == TY_U8  || k == TY_U16 || k == TY_U32  || k == TY_U64  ||
           k == TY_USIZE || k == TY_CHAR;
}

static int ty_is_float(TyKind k)
{
    return k == TY_F32 || k == TY_F64;
}

static int ty_int_bits(TyKind k)
{
    switch (k) {
        case TY_I8:  case TY_U8:  case TY_CHAR:  return 8;
        case TY_I16: case TY_U16:                return 16;
        case TY_I32: case TY_U32:                return 32;
        case TY_I64: case TY_U64: case TY_USIZE: return 64;
        default: return 0;
    }
}

/* 1 when the given i64 value fits in type k without data loss */
static int val_fits(long long v, TyKind k)
{
    switch (k) {
        case TY_I8:    return v >= -128LL              && v <= 127LL;
        case TY_I16:   return v >= -32768LL            && v <= 32767LL;
        case TY_I32:   return v >= -2147483648LL       && v <= 2147483647LL;
        case TY_I64:   return 1;
        case TY_U8:    return v >= 0 && v <= 255LL;
        case TY_U16:   return v >= 0 && v <= 65535LL;
        case TY_U32:   return v >= 0 && v <= 4294967295LL;
        case TY_U64:   return v >= 0;
        case TY_USIZE: return v >= 0;
        case TY_CHAR:  return v >= 0 && v <= 127LL;
        default:       return 1;
    }
}

/* Convert a type-name string to OlrnType — shared by type_from_ref and cast inference */
static OlrnType type_from_name(const char *n)
{
    if (!n) return OTY_UNKNOWN;
    if (strcmp(n, "void")   == 0) return OTY(TY_VOID);
    if (strcmp(n, "bool")   == 0) return OTY(TY_BOOL);
    if (strcmp(n, "char")   == 0) return OTY(TY_CHAR);
    if (strcmp(n, "i8")     == 0) return OTY(TY_I8);
    if (strcmp(n, "i16")    == 0) return OTY(TY_I16);
    if (strcmp(n, "i32")    == 0 || strcmp(n, "int")    == 0) return OTY(TY_I32);
    if (strcmp(n, "i64")    == 0 || strcmp(n, "long")   == 0) return OTY(TY_I64);
    if (strcmp(n, "u8")     == 0 || strcmp(n, "byte")   == 0 ||
        strcmp(n, "ubyte")  == 0) return OTY(TY_U8);
    if (strcmp(n, "u16")    == 0 || strcmp(n, "ushort") == 0 ||
        strcmp(n, "short")  == 0) return OTY(TY_U16);
    if (strcmp(n, "u32")    == 0 || strcmp(n, "uint")   == 0) return OTY(TY_U32);
    if (strcmp(n, "u64")    == 0 || strcmp(n, "ulong")  == 0) return OTY(TY_U64);
    if (strcmp(n, "usize")  == 0) return OTY(TY_USIZE);
    if (strcmp(n, "f32")    == 0 || strcmp(n, "float")  == 0) return OTY(TY_F32);
    if (strcmp(n, "f64")    == 0 || strcmp(n, "double") == 0) return OTY(TY_F64);
    if (strcmp(n, "str")    == 0 || strcmp(n, "mstr")   == 0 ||
        strcmp(n, "istr")   == 0) return OTY(TY_STR);
    return OTY_NAMED(n);
}

static OlrnType type_from_ref(AstNode *ref)
{
    if (!ref || ref->kind != NODE_TYPE_REF) return OTY_UNKNOWN;
    if (ref->type_ref.is_list) return (OlrnType){TY_LIST, ref->type_ref.name};
    if (ref->type_ref.is_map)  return (OlrnType){TY_MAP,  ref->type_ref.map_val};
    if (ref->type_ref.is_set)  return (OlrnType){TY_SET,  ref->type_ref.name};
    if (ref->type_ref.is_result || ref->type_ref.tuple.count > 0)
        return OTY_UNKNOWN;
    if (ref->type_ref.is_smart) return OTY(TY_SMART);
    if (ref->type_ref.is_ptr)   return OTY(TY_PTR);
    return type_from_name(ref->type_ref.name);
}

/* -----------------------------------------------------------------------
   Check context
   ----------------------------------------------------------------------- */

typedef struct {
    AstNode    *err_decls[MAX_ERR_SETS];
    int         err_decl_count;
    AstNode    *fns[MAX_FNS];
    int         fn_count;
    AstNode    *imports[MAX_IMPORTS];
    int         import_used[MAX_IMPORTS];
    int         import_count;
    /* symbol table */
    const char *syms[MAX_SYMS];
    int         sym_ptr_kind[MAX_SYMS]; /* 0=non-ptr, 1=*T, 2=^T */
    OlrnType    sym_otype[MAX_SYMS];    /* full type for each symbol */
    int         sym_count;
    int         scope_start[MAX_SCOPES];
    int         scope_depth;
    const char *fn_name;
    const char *fn_set;
    AstNode    *fn_ret_type;
    int         errors;
    int         warnings;
    int         check_shadow; /* 1 when declaring user variables (not fn params) */
    RegStruct   struct_reg[MAX_STRUCT_REG];
    int         struct_reg_count;
} Check;

static void push_scope(Check *c)
{
    if (c->scope_depth < MAX_SCOPES)
        c->scope_start[c->scope_depth] = c->sym_count;
    c->scope_depth++;
}

static void pop_scope(Check *c)
{
    c->scope_depth--;
    if (c->scope_depth < MAX_SCOPES)
        c->sym_count = c->scope_start[c->scope_depth];
}

static void declare(Check *c, const char *name, int line, int col,
                    int ptr_kind, OlrnType otype)
{
    if (!name || strcmp(name, "_") == 0) return;
    if (c->check_shadow) {
        for (int i = 0; i < c->import_count; i++) {
            if (strcmp(c->imports[i]->import_decl.alias, name) == 0) {
                fprintf(stderr,
                        "error:%d:%d: '%s' shadows the import alias "
                        "declared on line %d\n",
                        line, col, name, c->imports[i]->line);
                c->errors++;
                break;
            }
        }
    }
    if (c->sym_count < MAX_SYMS) {
        c->syms[c->sym_count]          = name;
        c->sym_ptr_kind[c->sym_count]  = ptr_kind;
        c->sym_otype[c->sym_count]     = otype;
        c->sym_count++;
    }
}

static int sym_ptr_kind(Check *c, const char *name)
{
    for (int i = c->sym_count - 1; i >= 0; i--)
        if (strcmp(c->syms[i], name) == 0)
            return c->sym_ptr_kind[i];
    return 0;
}

static OlrnType sym_type_of(Check *c, const char *name)
{
    for (int i = c->sym_count - 1; i >= 0; i--)
        if (strcmp(c->syms[i], name) == 0)
            return c->sym_otype[i];
    return OTY_UNKNOWN;
}

static int type_ptr_kind(AstNode *type)
{
    if (!type || type->kind != NODE_TYPE_REF) return 0;
    if (type->type_ref.is_smart) return 2;
    if (type->type_ref.is_ptr)   return 1;
    return 0;
}

static int var_decl_ptr_kind(AstNode *var)
{
    int pk = type_ptr_kind(var->var_decl.type_ref);
    if (pk) return pk;
    AstNode *init = var->var_decl.init;
    if (init && init->kind == NODE_BUILTIN_CALL &&
        strcmp(init->call.name, "alo") == 0)
        return 1;
    return 0;
}

static int is_local(Check *c, const char *name)
{
    for (int i = c->sym_count - 1; i >= 0; i--)
        if (strcmp(c->syms[i], name) == 0) return 1;
    return 0;
}

static void mark_import_used(Check *c, const char *name)
{
    if (is_local(c, name)) return;
    for (int i = 0; i < c->import_count; i++)
        if (strcmp(c->imports[i]->import_decl.alias, name) == 0)
            c->import_used[i] = 1;
}

static AstNode *find_err_set(Check *c, const char *name)
{
    for (int i = 0; i < c->err_decl_count; i++)
        if (strcmp(c->err_decls[i]->err_decl.name, name) == 0)
            return c->err_decls[i];
    return NULL;
}

static int set_has_variant(AstNode *set, const char *variant)
{
    for (int i = 0; i < set->err_decl.variants.count; i++)
        if (strcmp(set->err_decl.variants.items[i]->ident.name, variant) == 0)
            return 1;
    return 0;
}

static AstNode *find_fn(Check *c, const char *name)
{
    for (int i = 0; i < c->fn_count; i++)
        if (strcmp(c->fns[i]->fn_decl.name, name) == 0)
            return c->fns[i];
    return NULL;
}

static RegStruct *find_struct(Check *c, const char *name)
{
    for (int i = 0; i < c->struct_reg_count; i++)
        if (strcmp(c->struct_reg[i].sname, name) == 0)
            return &c->struct_reg[i];
    return NULL;
}

static RegField *find_field(Check *c, const char *sname, const char *fname)
{
    RegStruct *s = find_struct(c, sname);
    if (!s) return NULL;
    for (int i = 0; i < s->field_count; i++)
        if (strcmp(s->fields[i].fname, fname) == 0)
            return &s->fields[i];
    return NULL;
}

static const char *fn_err_set(AstNode *fn)
{
    AstNode *rt = fn->fn_decl.ret_type;
    if (!rt || !rt->type_ref.is_result) return NULL;
    return rt->type_ref.err_set;
}

static int type_is_void_result(AstNode *type)
{
    return type && type->kind == NODE_TYPE_REF &&
           type->type_ref.is_result &&
           type->type_ref.name &&
           strcmp(type->type_ref.name, "void") == 0;
}

static int type_is_any(AstNode *type)
{
    return type && type->kind == NODE_TYPE_REF && type->type_ref.name &&
           strcmp(type->type_ref.name, "any") == 0;
}

/* -----------------------------------------------------------------------
   Type inference and compatibility
   ----------------------------------------------------------------------- */

/* Forward-declared — walk calls type_of_expr, type_of_expr uses Check */
static OlrnType type_of_expr(Check *c, AstNode *n);

static OlrnType type_of_expr(Check *c, AstNode *n)
{
    if (!n) return OTY_UNKNOWN;
    switch (n->kind) {
        case NODE_INT_LIT:   return OTY(TY_I64);
        case NODE_FLOAT_LIT: return OTY(TY_F64);
        case NODE_STR_LIT:   return OTY(TY_STR);
        case NODE_BOOL_LIT:  return OTY(TY_BOOL);
        case NODE_CHAR_LIT:  return OTY(TY_CHAR);
        case NODE_NULL_LIT:  return OTY(TY_PTR);
        case NODE_IDENT:     return sym_type_of(c, n->ident.name);
        case NODE_CALL: {
            AstNode *fn = find_fn(c, n->call.name);
            return fn ? type_from_ref(fn->fn_decl.ret_type) : OTY_UNKNOWN;
        }
        case NODE_BUILTIN_CALL: {
            const char *bn = n->call.name;
            if (strcmp(bn, "alo") == 0)                               return OTY_UNKNOWN;
            if (strcmp(bn, "pid") == 0 || strcmp(bn, "cmd") == 0)    return OTY(TY_I32);
            if (strcmp(bn, "getenv") == 0 || strcmp(bn, "hex") == 0 ||
                strcmp(bn, "cin") == 0  || strcmp(bn, "type") == 0)  return OTY(TY_STR);
            if (strcmp(bn, "sizeof") == 0 || strcmp(bn, "alignof") == 0) return OTY(TY_USIZE);
            /* @ls.init(T, cap) / @map.init(K, V, cap) / @set.init(T, cap) */
            if (strncmp(bn, "ls.", 3) == 0 && n->call.args.count >= 1 &&
                n->call.args.items[0]->kind == NODE_IDENT)
                return (OlrnType){TY_LIST, n->call.args.items[0]->ident.name};
            if (strncmp(bn, "map.", 4) == 0 && n->call.args.count >= 2 &&
                n->call.args.items[1]->kind == NODE_IDENT)
                return (OlrnType){TY_MAP, n->call.args.items[1]->ident.name};
            if (strncmp(bn, "set.", 4) == 0 && n->call.args.count >= 1 &&
                n->call.args.items[0]->kind == NODE_IDENT)
                return (OlrnType){TY_SET, n->call.args.items[0]->ident.name};
            /* @i32(x), @f64(x) etc. — cast to target type; guard against unknown names */
            OlrnType ct = type_from_name(bn);
            return ct.kind == TY_NAMED ? OTY_UNKNOWN : ct;
        }
        case NODE_BINARY:
            switch (n->binary.op) {
                case TOK_EQEQ: case TOK_NEQ:
                case TOK_LT:   case TOK_GT:
                case TOK_LEQ:  case TOK_GEQ:
                case TOK_AND_KW: case TOK_OR_KW:
                    return OTY(TY_BOOL);
                default:
                    return type_of_expr(c, n->binary.left);
            }
        case NODE_UNARY:
            if (n->unary.op == '!')
                return OTY(TY_BOOL);
            if (n->unary.op == '-')
                return type_of_expr(c, n->unary.operand);
            return OTY_UNKNOWN;
        case NODE_FIELD:
        case NODE_FIELD_PTR: {
            OlrnType tt = type_of_expr(c, n->field.target);
            /* .len on str is always usize */
            if (strcmp(n->field.name, "len") == 0 && tt.kind == TY_STR)
                return OTY(TY_USIZE);
            if (tt.kind == TY_NAMED && tt.name) {
                RegField *rf = find_field(c, tt.name, n->field.name);
                if (rf) return rf->ftype;
            }
            return OTY_UNKNOWN;
        }
        case NODE_SUBSCRIPT: {
            OlrnType tt = type_of_expr(c, n->subscript.target);
            if (tt.kind == TY_LIST && tt.name) return type_from_name(tt.name);
            return OTY_UNKNOWN;
        }
        case NODE_CALL_EXPR: {
            AstNode *cel = n->call_expr.callee;
            if (cel->kind == NODE_FIELD) {
                OlrnType tt = type_of_expr(c, cel->field.target);
                const char *m = cel->field.name;
                if ((tt.kind == TY_LIST || tt.kind == TY_SET) && tt.name) {
                    if (strcmp(m, "get") == 0 || strcmp(m, "pop") == 0)
                        return type_from_name(tt.name);
                } else if (tt.kind == TY_MAP && tt.name) {
                    if (strcmp(m, "get") == 0)
                        return type_from_name(tt.name);
                }
            }
            return OTY_UNKNOWN;
        }
        case NODE_STRUCT_LIT:
            if (n->struct_lit.type_name)
                return OTY_NAMED(n->struct_lit.type_name);
            return OTY_UNKNOWN;
        case NODE_TRY_EXPR:
        case NODE_CATCH_EXPR:
            return type_of_expr(c, n->try_expr.expr);
        default:
            return OTY_UNKNOWN;
    }
}

/* Human-readable type label for diagnostics */
static const char *oty_label(OlrnType t)
{
    if (t.kind == TY_NAMED && t.name) return t.name;
    return ty_name(t.kind);
}

/* Check that 'from' type is compatible with 'to' type.
   Emits a warning/error and increments the appropriate counter.
   Returns 1 if compatible (no error), 0 if hard mismatch. */
static int check_compat(Check *c, OlrnType from, OlrnType to,
                         int line, int col, const char *ctx,
                         AstNode *init_node)
{
    if (from.kind == TY_UNKNOWN || to.kind == TY_UNKNOWN) return 1;
    if (from.kind == to.kind) {
        if (from.kind == TY_NAMED) {
            if (from.name && to.name && strcmp(from.name, to.name) != 0) {
                fprintf(stderr,
                        "error:%d:%d: type mismatch in %s: "
                        "got '%s', expected '%s'\n",
                        line, col, ctx, from.name, to.name);
                c->errors++;
                return 0;
            }
        }
        if (from.kind == TY_LIST || from.kind == TY_MAP || from.kind == TY_SET) {
            if (from.name && to.name && strcmp(from.name, to.name) != 0) {
                fprintf(stderr,
                        "error:%d:%d: type mismatch in %s: "
                        "got '%s(%s)', expected '%s(%s)'\n",
                        line, col, ctx,
                        ty_name(from.kind), from.name,
                        ty_name(to.kind), to.name);
                c->errors++;
                return 0;
            }
        }
        return 1;
    }
    /* int ↔ int */
    if (ty_is_int(from.kind) && ty_is_int(to.kind)) {
        int fb = ty_int_bits(from.kind);
        int tb = ty_int_bits(to.kind);
        if (fb > tb) {
            /* narrowing — if it's a literal we can check the value */
            if (init_node && init_node->kind == NODE_INT_LIT) {
                long long v = init_node->int_lit.value;
                if (!val_fits(v, to.kind)) {
                    fprintf(stderr,
                            "error:%d:%d: integer literal %lld is out of "
                            "range for '%s'\n",
                            line, col, v, ty_name(to.kind));
                    c->errors++;
                    return 0;
                }
                /* fits — no warning; codegen handles the cast */
                return 1;
            }
            fprintf(stderr,
                    "warning:%d:%d: narrowing in %s: "
                    "'%s' assigned to '%s' — use @%s(...) to cast\n",
                    line, col, ctx, ty_name(from.kind), ty_name(to.kind),
                    ty_name(to.kind));
            c->warnings++;
        }
        return 1;  /* widening is always fine */
    }
    /* float ↔ float */
    if (ty_is_float(from.kind) && ty_is_float(to.kind)) {
        if (from.kind == TY_F64 && to.kind == TY_F32) {
            /* float literal → f32: user explicitly wrote :f32 = 1.6, skip */
            if (init_node && init_node->kind == NODE_FLOAT_LIT) return 1;
            fprintf(stderr,
                    "warning:%d:%d: narrowing in %s: "
                    "f64 assigned to f32 — use @f32(...) to cast\n",
                    line, col, ctx);
            c->warnings++;
        }
        return 1;
    }
    /* int → float: implicit widening, always fine */
    if (ty_is_int(from.kind) && ty_is_float(to.kind)) return 1;
    /* target is a named type (type alias, struct, enum) — can't verify without resolution */
    if (to.kind == TY_NAMED) return 1;
    /* Everything else is a hard mismatch */
    fprintf(stderr,
            "error:%d:%d: type mismatch in %s: "
            "got '%s', expected '%s'\n",
            line, col, ctx, oty_label(from), oty_label(to));
    c->errors++;
    return 0;
}

/* -----------------------------------------------------------------------
   Error-handling checks (unchanged logic, now uses type model where relevant)
   ----------------------------------------------------------------------- */

static void check_ret_value(Check *c, AstNode *v, int line, int col)
{
    if (v->kind == NODE_ERR_LIT) {
        if (c->fn_set) {
            fprintf(stderr,
                    "error:%d:%d: fn '%s' returns '%s!...' — ad-hoc error "
                    "'err.%s' is not in set '%s'\n",
                    line, col, c->fn_name, c->fn_set,
                    v->err_lit.variant_name, c->fn_set);
            c->errors++;
        }
        return;
    }
    if (v->kind == NODE_FIELD && v->field.target->kind == NODE_IDENT) {
        AstNode *set = find_err_set(c, v->field.target->ident.name);
        if (!set) return;
        if (!set_has_variant(set, v->field.name)) {
            fprintf(stderr,
                    "error:%d:%d: error set '%s' has no variant '%s'\n",
                    line, col, set->err_decl.name, v->field.name);
            c->errors++;
        }
        if (c->fn_set && strcmp(set->err_decl.name, c->fn_set) != 0) {
            fprintf(stderr,
                    "error:%d:%d: fn '%s' returns '%s!...' but error is "
                    "from set '%s'\n",
                    line, col, c->fn_name, c->fn_set, set->err_decl.name);
            c->errors++;
        }
    }
}

/* -----------------------------------------------------------------------
   AST walk
   ----------------------------------------------------------------------- */

static void walk(Check *c, AstNode *n);

static void walk_list(Check *c, NodeList *l)
{
    for (int i = 0; i < l->count; i++) walk(c, l->items[i]);
}

static void walk(Check *c, AstNode *n)
{
    if (!n) return;
    switch (n->kind) {
        case NODE_BLOCK:
            push_scope(c);
            walk_list(c, &n->block.stmts);
            pop_scope(c);
            break;

        case NODE_RET:
            if (n->ret.value) {
                if (!c->fn_ret_type) {
                    fprintf(stderr,
                            "error:%d:%d: fn '%s' returns void; add an "
                            "explicit return type or use bare 'ret'\n",
                            n->line, n->col, c->fn_name);
                    c->errors++;
                } else {
                    OlrnType declared = type_from_ref(c->fn_ret_type);
                    OlrnType actual   = type_of_expr(c, n->ret.value);
                    check_compat(c, actual, declared,
                                 n->line, n->col, "return", n->ret.value);
                }
                check_ret_value(c, n->ret.value, n->line, n->col);
                walk(c, n->ret.value);
            } else if (c->fn_ret_type && !type_is_void_result(c->fn_ret_type)) {
                fprintf(stderr,
                        "error:%d:%d: fn '%s' returns a value; bare 'ret' "
                        "is only valid for void returns\n",
                        n->line, n->col, c->fn_name);
                c->errors++;
            }
            break;

        case NODE_TRY_EXPR: {
            AstNode *e = n->try_expr.expr;
            if (c->fn_set && e->kind == NODE_CALL) {
                AstNode *callee = find_fn(c, e->call.name);
                const char *cs = callee ? fn_err_set(callee) : NULL;
                if (cs && strcmp(cs, c->fn_set) != 0) {
                    fprintf(stderr,
                            "error:%d:%d: fn '%s' returns '%s!...' but "
                            "'try %s(...)' propagates set '%s'\n",
                            n->line, n->col, c->fn_name, c->fn_set, e->call.name, cs);
                    c->errors++;
                }
            }
            walk(c, e);
            break;
        }

        case NODE_CATCH_EXPR:
            walk(c, n->catch_expr.expr);
            walk(c, n->catch_expr.fallback);
            if (n->catch_expr.body) {
                push_scope(c);
                c->check_shadow = 1;
                declare(c, n->catch_expr.err_var, n->line, n->col, 0, OTY_UNKNOWN);
                c->check_shadow = 0;
                walk(c, n->catch_expr.body);
                pop_scope(c);
            }
            break;

        case NODE_ASSIGN:
            if (n->assign.rhs && n->assign.rhs->kind == NODE_NULL_LIT &&
                n->assign.lhs && n->assign.lhs->kind == NODE_IDENT &&
                is_local(c, n->assign.lhs->ident.name) &&
                sym_ptr_kind(c, n->assign.lhs->ident.name) == 0) {
                fprintf(stderr,
                        "error:%d:%d: cannot assign null to non-pointer '%s'\n",
                        n->line, n->col, n->assign.lhs->ident.name);
                c->errors++;
            }
            if (n->assign.lhs && n->assign.rhs) {
                OlrnType lhs_t = type_of_expr(c, n->assign.lhs);
                OlrnType rhs_t = type_of_expr(c, n->assign.rhs);
                check_compat(c, rhs_t, lhs_t,
                             n->line, n->col, "assignment", n->assign.rhs);
            }
            walk(c, n->assign.lhs);
            walk(c, n->assign.rhs);
            break;

        case NODE_WHILE:
            walk(c, n->while_loop.cond); walk(c, n->while_loop.body);
            break;

        case NODE_LOOP:
            push_scope(c);
            walk(c, n->loop_stmt.init); walk(c, n->loop_stmt.cond);
            walk(c, n->loop_stmt.step); walk(c, n->loop_stmt.body);
            pop_scope(c);
            break;

        case NODE_FOR_RANGE:
            walk(c, n->for_range.lo); walk(c, n->for_range.hi);
            walk(c, n->for_range.body);
            break;

        case NODE_FOR_EACH: {
            walk(c, n->for_each.iter);
            OlrnType iter_t = type_of_expr(c, n->for_each.iter);
            OlrnType elem_t = OTY_UNKNOWN;
            OlrnType idx_t  = OTY(TY_USIZE);
            if ((iter_t.kind == TY_LIST || iter_t.kind == TY_SET) && iter_t.name)
                elem_t = type_from_name(iter_t.name);
            else if (iter_t.kind == TY_MAP && iter_t.name && n->for_each.is_kv)
                idx_t = type_from_name(iter_t.name);  /* val type goes to idx slot */
            else if (iter_t.kind == TY_MAP && iter_t.name && !n->for_each.is_kv)
                elem_t = type_from_name(iter_t.name);
            push_scope(c);
            c->check_shadow = 1;
            declare(c, n->for_each.elem, n->line, n->col, 0, elem_t);
            declare(c, n->for_each.idx,  n->line, n->col, 0, idx_t);
            c->check_shadow = 0;
            walk(c, n->for_each.body);
            pop_scope(c);
            break;
        }

        case NODE_VAR_DECL: {
            walk(c, n->var_decl.init);
            OlrnType decl_t = type_from_ref(n->var_decl.type_ref);
            OlrnType init_t = type_of_expr(c, n->var_decl.init);
            /* type-compatibility check when type is explicit */
            if (n->var_decl.type_ref && n->var_decl.init) {
                check_compat(c, init_t, decl_t,
                             n->line, n->col, "assignment", n->var_decl.init);
            }
            /* null pointer check */
            if (n->var_decl.init && n->var_decl.init->kind == NODE_NULL_LIT &&
                var_decl_ptr_kind(n) == 0) {
                fprintf(stderr,
                        "error:%d:%d: null requires an explicit pointer type (*T or ^T)\n",
                        n->line, n->col);
                c->errors++;
            }
            /* inferred type: prefer declared; fall back to init type */
            OlrnType stored_t = (decl_t.kind != TY_UNKNOWN) ? decl_t : init_t;
            c->check_shadow = 1;
            declare(c, n->var_decl.name, n->line, n->col,
                    var_decl_ptr_kind(n), stored_t);
            c->check_shadow = 0;
            break;
        }

        case NODE_VAR_DECL_GROUP:
            for (int i = 0; i < n->var_decl_group.entries.count; i++) {
                AstNode *e = n->var_decl_group.entries.items[i];
                walk(c, e->var_decl.init);
                OlrnType grp_t = type_from_ref(n->var_decl_group.type_ref);
                c->check_shadow = 1;
                declare(c, e->var_decl.name, e->line, e->col,
                        type_ptr_kind(n->var_decl_group.type_ref), grp_t);
                c->check_shadow = 0;
            }
            break;

        case NODE_IF:
            walk(c, n->if_expr.cond); walk(c, n->if_expr.then_block);
            walk(c, n->if_expr.else_block);
            break;

        case NODE_WHEN:
            walk(c, n->when_expr.subject); walk_list(c, &n->when_expr.arms);
            break;

        case NODE_WHEN_ARM:
            walk(c, n->when_arm.pattern); walk(c, n->when_arm.body);
            break;

        case NODE_BINARY:
            walk(c, n->binary.left); walk(c, n->binary.right);
            break;

        case NODE_UNARY:      walk(c, n->unary.operand); break;
        case NODE_FIELD:
        case NODE_FIELD_PTR:  walk(c, n->field.target); break;
        case NODE_SUBSCRIPT:  walk(c, n->subscript.target); walk(c, n->subscript.index); break;
        case NODE_DEREF:      walk(c, n->deref.target); break;
        case NODE_DEFER:      walk(c, n->defer_stmt.expr); break;
        case NODE_ERRDEFER:   walk(c, n->errdefer_stmt.expr); break;

        case NODE_BUILTIN_CALL:
            if (strcmp(n->call.name, "free") == 0 && n->call.args.count == 1) {
                AstNode *arg = n->call.args.items[0];
                if (arg->kind == NODE_IDENT && is_local(c, arg->ident.name)) {
                    int pk = sym_ptr_kind(c, arg->ident.name);
                    if (pk == 2) {
                        fprintf(stderr,
                                "error:%d:%d: '@free' called on '%s' which is a "
                                "smart pointer (^T) — smart pointers free themselves; "
                                "remove the @free call\n",
                                n->line, n->col, arg->ident.name);
                        c->errors++;
                    } else if (pk == 0) {
                        fprintf(stderr,
                                "error:%d:%d: '@free' expects a raw pointer (*T), "
                                "but '%s' is not a pointer\n",
                                n->line, n->col, arg->ident.name);
                        c->errors++;
                    }
                }
            }
            walk_list(c, &n->call.args);
            break;

        case NODE_CALL: {
            AstNode *callee = find_fn(c, n->call.name);
            if (callee) {
                if (n->call.args.count != callee->fn_decl.params.count) {
                    fprintf(stderr,
                            "error:%d:%d: fn '%s' expects %d argument%s, got %d\n",
                            n->line, n->col, n->call.name,
                            callee->fn_decl.params.count,
                            callee->fn_decl.params.count == 1 ? "" : "s",
                            n->call.args.count);
                    c->errors++;
                } else {
                    /* type-check each argument */
                    for (int i = 0; i < callee->fn_decl.params.count; i++) {
                        AstNode *pm  = callee->fn_decl.params.items[i];
                        OlrnType pt  = type_from_ref(pm->param.type);
                        OlrnType at  = type_of_expr(c, n->call.args.items[i]);
                        check_compat(c, at, pt,
                                     n->line, n->col, "argument",
                                     n->call.args.items[i]);
                    }
                }
            }
            walk_list(c, &n->call.args);
            break;
        }

        case NODE_CALL_EXPR: {
            walk(c, n->call_expr.callee);
            walk_list(c, &n->call_expr.args);
            AstNode *cel = n->call_expr.callee;
            if (cel->kind != NODE_FIELD) break;
            OlrnType tt = type_of_expr(c, cel->field.target);
            const char *meth = cel->field.name;
            if ((tt.kind == TY_LIST || tt.kind == TY_SET) && tt.name) {
                OlrnType et = type_from_name(tt.name);
                if (strcmp(meth, "add") == 0 && n->call_expr.args.count == 1)
                    check_compat(c, type_of_expr(c, n->call_expr.args.items[0]),
                                 et, n->line, n->col, "list.add",
                                 n->call_expr.args.items[0]);
                if (strcmp(meth, "insert") == 0 && n->call_expr.args.count == 2)
                    check_compat(c, type_of_expr(c, n->call_expr.args.items[1]),
                                 et, n->line, n->col, "list.insert",
                                 n->call_expr.args.items[1]);
            } else if (tt.kind == TY_MAP && tt.name) {
                OlrnType vt = type_from_name(tt.name);
                if (strcmp(meth, "set") == 0 && n->call_expr.args.count == 2)
                    check_compat(c, type_of_expr(c, n->call_expr.args.items[1]),
                                 vt, n->line, n->col, "map.set",
                                 n->call_expr.args.items[1]);
            }
            break;
        }

        case NODE_ARRAY_LIT:  walk_list(c, &n->array_lit.elems); break;
        case NODE_STRUCT_LIT: {
            walk_list(c, &n->struct_lit.fields);
            if (!n->struct_lit.type_name) break;
            RegStruct *rs = find_struct(c, n->struct_lit.type_name);
            if (!rs) break;
            for (int i = 0; i < n->struct_lit.fields.count; i++) {
                AstNode *fi = n->struct_lit.fields.items[i];
                if (fi->kind != NODE_FIELD_INIT) continue;
                RegField *rf = find_field(c, rs->sname, fi->field_init.name);
                if (!rf) {
                    fprintf(stderr,
                            "error:%d:%d: struct '%s' has no field '%s'\n",
                            fi->line, fi->col, rs->sname, fi->field_init.name);
                    c->errors++;
                } else {
                    OlrnType vt = type_of_expr(c, fi->field_init.value);
                    check_compat(c, vt, rf->ftype,
                                 fi->line, fi->col, "struct field", fi->field_init.value);
                }
            }
            break;
        }
        case NODE_FIELD_INIT: walk(c, n->field_init.value); break;
        case NODE_IDENT:      mark_import_used(c, n->ident.name); break;
        default: break;
    }
}

static void check_fn_body(Check *c, AstNode *fn)
{
    c->fn_name = fn->fn_decl.name;
    c->fn_set  = fn_err_set(fn);
    c->fn_ret_type = fn->fn_decl.ret_type;
    push_scope(c);
    for (int j = 0; j < fn->fn_decl.params.count; j++) {
        AstNode *pm = fn->fn_decl.params.items[j];
        declare(c, pm->param.name, pm->line, pm->col,
                type_ptr_kind(pm->param.type), type_from_ref(pm->param.type));
    }
    walk(c, fn->fn_decl.body);
    pop_scope(c);
    c->fn_ret_type = NULL;
}

static int check_module(AstNode *mod)
{
    Check c = {0};

    /* first pass — register err sets, fns, structs */
    for (int i = 0; i < mod->module.decls.count; i++) {
        AstNode *d = mod->module.decls.items[i];
        if (d->kind == NODE_ERR_DECL && c.err_decl_count < MAX_ERR_SETS) {
            c.err_decls[c.err_decl_count++] = d;
        } else if (d->kind == NODE_FN_DECL && c.fn_count < MAX_FNS) {
            c.fns[c.fn_count++] = d;
        } else if (d->kind == NODE_STRUCT_DECL) {
            if (c.struct_reg_count < MAX_STRUCT_REG) {
                RegStruct *rs = &c.struct_reg[c.struct_reg_count++];
                rs->sname = d->struct_decl.name;
                rs->field_count = 0;
                for (int j = 0; j < d->struct_decl.fields.count &&
                                rs->field_count < MAX_FIELD_REG; j++) {
                    AstNode *f = d->struct_decl.fields.items[j];
                    rs->fields[rs->field_count].fname = f->param.name;
                    rs->fields[rs->field_count].ftype = type_from_ref(f->param.type);
                    rs->field_count++;
                }
            }
        }
    }

    /* check fn bodies */
    for (int i = 0; i < c.fn_count; i++)
        check_fn_body(&c, c.fns[i]);

    /* check struct methods */
    for (int i = 0; i < mod->module.decls.count; i++) {
        AstNode *d = mod->module.decls.items[i];
        if (d->kind != NODE_STRUCT_DECL) continue;
        for (int j = 0; j < d->struct_decl.methods.count; j++)
            check_fn_body(&c, d->struct_decl.methods.items[j]);
    }

    if (c.warnings > 0)
        fprintf(stderr, "%d warning%s in module '%s'\n",
                c.warnings, c.warnings == 1 ? "" : "s", mod->module.name);
    return c.errors;
}

int check_program(AstNode *program)
{
    Check c = {0};

    for (int i = 0; i < program->program.imports.count; i++) {
        if (c.import_count < MAX_IMPORTS)
            c.imports[c.import_count++] = program->program.imports.items[i];
    }

    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *d = program->program.decls.items[i];
        if (d->kind == NODE_ERR_DECL && c.err_decl_count < MAX_ERR_SETS) {
            c.err_decls[c.err_decl_count++] = d;
        } else if (d->kind == NODE_FN_DECL && c.fn_count < MAX_FNS) {
            AstNode *prev = find_fn(&c, d->fn_decl.name);
            if (prev) {
                fprintf(stderr,
                        "error:%d:%d: duplicate fn '%s' (first declared on line %d)\n",
                        d->line, d->col, d->fn_decl.name, prev->line);
                c.errors++;
            } else {
                c.fns[c.fn_count++] = d;
            }
        } else if (d->kind == NODE_STRUCT_DECL) {
            /* register fields for type_of_expr resolution */
            if (c.struct_reg_count < MAX_STRUCT_REG) {
                RegStruct *rs = &c.struct_reg[c.struct_reg_count++];
                rs->sname = d->struct_decl.name;
                rs->field_count = 0;
                for (int j = 0; j < d->struct_decl.fields.count &&
                                rs->field_count < MAX_FIELD_REG; j++) {
                    AstNode *f = d->struct_decl.fields.items[j];
                    rs->fields[rs->field_count].fname = f->param.name;
                    rs->fields[rs->field_count].ftype = type_from_ref(f->param.type);
                    rs->field_count++;
                }
            }
            for (int j = 0; j < d->struct_decl.fields.count; j++) {
                AstNode *f = d->struct_decl.fields.items[j];
                if (strcmp(f->param.name, "len") == 0) {
                    fprintf(stderr,
                            "error:%d:%d: struct '%s': field name 'len' "
                            "is reserved (the .len length property)\n",
                            f->line, f->col, d->struct_decl.name);
                    c.errors++;
                }
                if (type_is_any(f->param.type)) {
                    fprintf(stderr,
                            "error:%d:%d: struct '%s': field '%s' uses "
                            "'any', but generic struct fields are not "
                            "implemented yet; use a concrete type\n",
                            f->line, f->col, d->struct_decl.name, f->param.name);
                    c.errors++;
                }
            }
        }
    }

    c.fn_name = "<top-level>";
    c.fn_set  = NULL;
    c.fn_ret_type = NULL;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *d = program->program.decls.items[i];
        if (d->kind == NODE_VAR_DECL || d->kind == NODE_VAR_DECL_GROUP)
            walk(&c, d);
    }

    for (int i = 0; i < c.fn_count; i++)
        check_fn_body(&c, c.fns[i]);

    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *d = program->program.decls.items[i];
        if (d->kind != NODE_STRUCT_DECL) continue;
        for (int j = 0; j < d->struct_decl.methods.count; j++)
            check_fn_body(&c, d->struct_decl.methods.items[j]);
    }

    for (int i = 0; i < c.import_count; i++) {
        if (!c.import_used[i]) {
            AstNode *imp = c.imports[i];
            fprintf(stderr, "error:%d:%d: unused import '%s'\n",
                    imp->line, imp->col, imp->import_decl.alias);
            c.errors++;
        }
    }
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *d = program->program.decls.items[i];
        if (d->kind == NODE_MODULE)
            c.errors += check_module(d);
    }

    if (c.warnings > 0)
        fprintf(stderr, "%d warning%s\n", c.warnings, c.warnings == 1 ? "" : "s");
    return c.errors;
}
