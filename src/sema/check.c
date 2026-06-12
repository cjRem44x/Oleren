#include "check.h"
#include <stdio.h>
#include <string.h>

#define MAX_ERR_SETS 64
#define MAX_FNS      1024
#define MAX_IMPORTS  64
#define MAX_SYMS     1024
#define MAX_SCOPES   128

typedef struct {
    AstNode    *err_decls[MAX_ERR_SETS];
    int         err_decl_count;
    AstNode    *fns[MAX_FNS];
    int         fn_count;
    AstNode    *imports[MAX_IMPORTS];
    int         import_used[MAX_IMPORTS];
    int         import_count;
    /* symbol table — a stack of local names (vars, params, bindings),
       so a local that shadows an import alias doesn't count as a use */
    const char *syms[MAX_SYMS];
    int         sym_is_smart[MAX_SYMS]; /* 1 if declared as ^T */
    int         sym_count;
    int         scope_start[MAX_SCOPES];
    int         scope_depth;
    const char *fn_name; /* fn being walked */
    const char *fn_set;  /* its declared err set; NULL = generic !T or no result */
    int         errors;
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

static void declare(Check *c, const char *name, int line, int is_smart)
{
    if (!name || strcmp(name, "_") == 0) return;
    /* shadowing an import alias is banned: codegen treats alias-rooted
       field chains as module access, so a shadow would silently emit
       wrong code (local.field would become a stripped module name) */
    for (int i = 0; i < c->import_count; i++) {
        if (strcmp(c->imports[i]->import_decl.alias, name) == 0) {
            fprintf(stderr,
                    "error: line %d: '%s' shadows the import alias "
                    "declared on line %d\n",
                    line, name, c->imports[i]->line);
            c->errors++;
            break;
        }
    }
    if (c->sym_count < MAX_SYMS) {
        c->syms[c->sym_count]          = name;
        c->sym_is_smart[c->sym_count]  = is_smart;
        c->sym_count++;
    }
}

static int sym_is_smart(Check *c, const char *name)
{
    for (int i = c->sym_count - 1; i >= 0; i--)
        if (strcmp(c->syms[i], name) == 0)
            return c->sym_is_smart[i];
    return 0;
}

static int is_local(Check *c, const char *name)
{
    for (int i = c->sym_count - 1; i >= 0; i--)
        if (strcmp(c->syms[i], name) == 0) return 1;
    return 0;
}

/* an alias ident appeared in an expression — the import is used,
   unless a local binding shadows the alias */
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
        case NODE_BLOCK:
            push_scope(c);
            walk_list(c, &n->block.stmts);
            pop_scope(c);
            break;
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
            if (n->catch_expr.body) {
                push_scope(c);
                declare(c, n->catch_expr.err_var, n->line, 0);
                walk(c, n->catch_expr.body);
                pop_scope(c);
            }
            break;
        case NODE_ASSIGN:    walk(c, n->assign.lhs); walk(c, n->assign.rhs); break;
        case NODE_WHILE:     walk(c, n->while_loop.cond); walk(c, n->while_loop.body); break;
        case NODE_LOOP:
            push_scope(c); /* loop init declares into the loop's scope */
            walk(c, n->loop_stmt.init); walk(c, n->loop_stmt.cond);
            walk(c, n->loop_stmt.step); walk(c, n->loop_stmt.body);
            pop_scope(c);
            break;
        case NODE_FOR_RANGE: walk(c, n->for_range.lo); walk(c, n->for_range.hi); walk(c, n->for_range.body); break;
        case NODE_FOR_EACH:
            walk(c, n->for_each.iter);
            push_scope(c);
            declare(c, n->for_each.elem, n->line, 0);
            declare(c, n->for_each.idx,  n->line, 0);
            walk(c, n->for_each.body);
            pop_scope(c);
            break;
        case NODE_VAR_DECL: {
            walk(c, n->var_decl.init);   /* init may reference outer names */
            int smart = n->var_decl.type_ref && n->var_decl.type_ref->type_ref.is_smart;
            declare(c, n->var_decl.name, n->line, smart);
            break;
        }
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
            if (strcmp(n->call.name, "free") == 0 && n->call.args.count == 1) {
                AstNode *arg = n->call.args.items[0];
                if (arg->kind == NODE_IDENT && sym_is_smart(c, arg->ident.name)) {
                    fprintf(stderr,
                            "error: line %d: '@free' called on '%s' which is a "
                            "smart pointer (^T) — smart pointers free themselves; "
                            "remove the @free call\n",
                            n->line, arg->ident.name);
                    c->errors++;
                }
            }
            walk_list(c, &n->call.args);
            break;
        case NODE_CALL:      walk_list(c, &n->call.args); break;
        case NODE_CALL_EXPR: walk(c, n->call_expr.callee); walk_list(c, &n->call_expr.args); break;
        case NODE_ARRAY_LIT: walk_list(c, &n->array_lit.elems); break;
        case NODE_STRUCT_LIT: walk_list(c, &n->struct_lit.fields); break;
        case NODE_FIELD_INIT: walk(c, n->field_init.value); break;
        case NODE_IDENT:     mark_import_used(c, n->ident.name); break;
        default: break; /* literals, decls — no statements inside */
    }
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
        if (d->kind == NODE_ERR_DECL && c.err_decl_count < MAX_ERR_SETS)
            c.err_decls[c.err_decl_count++] = d;
        else if (d->kind == NODE_FN_DECL && c.fn_count < MAX_FNS)
            c.fns[c.fn_count++] = d;
        else if (d->kind == NODE_STRUCT_DECL) {
            /* 'len' is reserved — it's the length property on str/arrays */
            for (int j = 0; j < d->struct_decl.fields.count; j++) {
                AstNode *f = d->struct_decl.fields.items[j];
                if (strcmp(f->param.name, "len") == 0) {
                    fprintf(stderr,
                            "error: line %d: struct '%s': field name 'len' "
                            "is reserved (the .len length property)\n",
                            f->line, d->struct_decl.name);
                    c.errors++;
                }
            }
        }
    }

    /* top-level constants/vars can reference import aliases too */
    c.fn_name = "<top-level>";
    c.fn_set  = NULL;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *d = program->program.decls.items[i];
        if (d->kind == NODE_VAR_DECL || d->kind == NODE_VAR_DECL_GROUP)
            walk(&c, d);
    }

    for (int i = 0; i < c.fn_count; i++) {
        AstNode *fn = c.fns[i];
        c.fn_name = fn->fn_decl.name;
        c.fn_set  = fn_err_set(fn);
        push_scope(&c);
        for (int j = 0; j < fn->fn_decl.params.count; j++) {
            AstNode *pm = fn->fn_decl.params.items[j];
            int smart = pm->param.type && pm->param.type->type_ref.is_smart;
            declare(&c, pm->param.name, pm->line, smart);
        }
        walk(&c, fn->fn_decl.body);
        pop_scope(&c);
    }

    for (int i = 0; i < c.import_count; i++) {
        if (!c.import_used[i]) {
            AstNode *imp = c.imports[i];
            fprintf(stderr, "error: line %d: unused import '%s'\n",
                    imp->line, imp->import_decl.alias);
            c.errors++;
        }
    }
    return c.errors;
}
