#include "check.h"
#include <stdio.h>
#include <string.h>

#define MAX_ERR_SETS 64
#define MAX_FNS      1024

typedef struct {
    AstNode    *err_decls[MAX_ERR_SETS];
    int         err_decl_count;
    AstNode    *fns[MAX_FNS];
    int         fn_count;
    const char *fn_name; /* fn being walked */
    const char *fn_set;  /* its declared err set; NULL = generic !T or no result */
    int         errors;
} Check;

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

/* declared err set of a fn's return type; NULL = generic !T or non-result */
static const char *fn_err_set(AstNode *fn)
{
    AstNode *rt = fn->fn_decl.ret_type;
    if (!rt || !rt->type_ref.is_result) return NULL;
    return rt->type_ref.err_set;
}

static void walk(Check *c, AstNode *n);

static void walk_list(Check *c, NodeList *l)
{
    for (int i = 0; i < l->count; i++) walk(c, l->items[i]);
}

/* validate an error value appearing in 'ret' */
static void check_ret_value(Check *c, AstNode *v, int line)
{
    if (v->kind == NODE_ERR_LIT) {
        if (c->fn_set) {
            fprintf(stderr,
                    "error: line %d: fn '%s' returns '%s!...' — ad-hoc error "
                    "'err.%s' is not in set '%s'\n",
                    line, c->fn_name, c->fn_set,
                    v->err_lit.variant_name, c->fn_set);
            c->errors++;
        }
        return;
    }
    if (v->kind == NODE_FIELD && v->field.target->kind == NODE_IDENT) {
        AstNode *set = find_err_set(c, v->field.target->ident.name);
        if (!set) return; /* not an error value (enum, struct field, ...) */
        if (!set_has_variant(set, v->field.name)) {
            fprintf(stderr,
                    "error: line %d: error set '%s' has no variant '%s'\n",
                    line, set->err_decl.name, v->field.name);
            c->errors++;
        }
        if (c->fn_set && strcmp(set->err_decl.name, c->fn_set) != 0) {
            fprintf(stderr,
                    "error: line %d: fn '%s' returns '%s!...' but error is "
                    "from set '%s'\n",
                    line, c->fn_name, c->fn_set, set->err_decl.name);
            c->errors++;
        }
    }
}

static void walk(Check *c, AstNode *n)
{
    if (!n) return;
    switch (n->kind) {
        case NODE_BLOCK:    walk_list(c, &n->block.stmts); break;
        case NODE_RET:
            if (n->ret.value) {
                check_ret_value(c, n->ret.value, n->line);
                walk(c, n->ret.value);
            }
            break;
        case NODE_TRY_EXPR: {
            /* try'd call must not propagate a different named set */
            AstNode *e = n->try_expr.expr;
            if (c->fn_set && e->kind == NODE_CALL) {
                AstNode *callee = find_fn(c, e->call.name);
                const char *cs = callee ? fn_err_set(callee) : NULL;
                if (cs && strcmp(cs, c->fn_set) != 0) {
                    fprintf(stderr,
                            "error: line %d: fn '%s' returns '%s!...' but "
                            "'try %s(...)' propagates set '%s'\n",
                            n->line, c->fn_name, c->fn_set, e->call.name, cs);
                    c->errors++;
                }
            }
            walk(c, e);
            break;
        }
        case NODE_CATCH_EXPR:
            walk(c, n->catch_expr.expr);
            walk(c, n->catch_expr.fallback);
            walk(c, n->catch_expr.body);
            break;
        case NODE_ASSIGN:    walk(c, n->assign.lhs); walk(c, n->assign.rhs); break;
        case NODE_WHILE:     walk(c, n->while_loop.cond); walk(c, n->while_loop.body); break;
        case NODE_LOOP:
            walk(c, n->loop_stmt.init); walk(c, n->loop_stmt.cond);
            walk(c, n->loop_stmt.step); walk(c, n->loop_stmt.body);
            break;
        case NODE_FOR_RANGE: walk(c, n->for_range.lo); walk(c, n->for_range.hi); walk(c, n->for_range.body); break;
        case NODE_FOR_EACH:  walk(c, n->for_each.iter); walk(c, n->for_each.body); break;
        case NODE_VAR_DECL:  walk(c, n->var_decl.init); break;
        case NODE_VAR_DECL_GROUP: walk_list(c, &n->var_decl_group.entries); break;
        case NODE_IF:
            walk(c, n->if_expr.cond); walk(c, n->if_expr.then_block);
            walk(c, n->if_expr.else_block);
            break;
        case NODE_WHEN:      walk(c, n->when_expr.subject); walk_list(c, &n->when_expr.arms); break;
        case NODE_WHEN_ARM:  walk(c, n->when_arm.pattern); walk(c, n->when_arm.body); break;
        case NODE_BINARY:    walk(c, n->binary.left); walk(c, n->binary.right); break;
        case NODE_UNARY:     walk(c, n->unary.operand); break;
        case NODE_FIELD:
        case NODE_FIELD_PTR: walk(c, n->field.target); break;
        case NODE_SUBSCRIPT: walk(c, n->subscript.target); walk(c, n->subscript.index); break;
        case NODE_DEREF:     walk(c, n->deref.target); break;
        case NODE_DEFER:     walk(c, n->defer_stmt.expr); break;
        case NODE_ERRDEFER:  walk(c, n->errdefer_stmt.expr); break;
        case NODE_BUILTIN_CALL:
        case NODE_CALL:      walk_list(c, &n->call.args); break;
        case NODE_CALL_EXPR: walk(c, n->call_expr.callee); walk_list(c, &n->call_expr.args); break;
        case NODE_ARRAY_LIT: walk_list(c, &n->array_lit.elems); break;
        case NODE_STRUCT_LIT: walk_list(c, &n->struct_lit.fields); break;
        case NODE_FIELD_INIT: walk(c, n->field_init.value); break;
        default: break; /* literals, idents, decls — no statements inside */
    }
}

int check_program(AstNode *program)
{
    Check c = {0};

    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *d = program->program.decls.items[i];
        if (d->kind == NODE_ERR_DECL && c.err_decl_count < MAX_ERR_SETS)
            c.err_decls[c.err_decl_count++] = d;
        else if (d->kind == NODE_FN_DECL && c.fn_count < MAX_FNS)
            c.fns[c.fn_count++] = d;
    }

    for (int i = 0; i < c.fn_count; i++) {
        AstNode *fn = c.fns[i];
        c.fn_name = fn->fn_decl.name;
        c.fn_set  = fn_err_set(fn);
        walk(&c, fn->fn_decl.body);
    }
    return c.errors;
}
