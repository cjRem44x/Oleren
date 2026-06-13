#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void parser_init(Parser *p, Lexer *l)
{
    p->lexer     = l;
    p->had_error = 0;
    p->cur       = lexer_next(l);
    p->peek      = lexer_next(l);
}

/* consume current token, slide the window forward */
static Token next_tok(Parser *p)
{
    Token prev = p->cur;
    p->cur  = p->peek;
    p->peek = lexer_next(p->lexer);
    return prev;
}

static int check(Parser *p, TokenType t) { return p->cur.type == t; }

static int match(Parser *p, TokenType t)
{
    if (check(p, t)) { next_tok(p); return 1; }
    return 0;
}

static void parse_err(Parser *p, const char *msg)
{
    fprintf(stderr, "error:%d: %s (got '%.*s')\n",
            p->cur.line, msg, p->cur.len, p->cur.start);
    p->had_error = 1;
}

static Token expect(Parser *p, TokenType t)
{
    if (!check(p, t)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected '%s'", tok_type_name(t));
        parse_err(p, buf);
    }
    return next_tok(p);
}

/* copy token text into a heap-allocated C string */
static char *tok_dup(Token t)
{
    char *s = malloc(t.len + 1);
    memcpy(s, t.start, t.len);
    s[t.len] = '\0';
    return s;
}

/* forward declarations */
static AstNode *parse_expr_bp(Parser *p, int min_bp);
static AstNode *parse_block(Parser *p);
static AstNode *parse_if_chain(Parser *p);
static AstNode *parse_when(Parser *p);
static AstNode *parse_brace_literal(Parser *p);
static AstNode *parse_err_decl(Parser *p);
static AstNode *parse_fn_decl(Parser *p);

/* Consume zero or more implicit newlines / explicit semicolons. */
static void skip_newlines(Parser *p)
{
    while (p->cur.type == TOK_NEWLINE || p->cur.type == TOK_SEMICOLON)
        next_tok(p);
}

static AstNode *parse_expr(Parser *p) { return parse_expr_bp(p, 0); }

static AstNode *parse_type(Parser *p)
{
    AstNode *n = ast_node_new(NODE_TYPE_REF, p->cur.line);

    /* !T or ErrSet!T error-union prefix */
    if (match(p, TOK_BANG)) {
        n->type_ref.is_result = 1;
    } else if (check(p, TOK_IDENT) && p->peek.type == TOK_BANG) {
        n->type_ref.err_set = tok_dup(p->cur); /* set constraint, checked in sema */
        next_tok(p); /* consume ErrSet name */
        next_tok(p); /* consume ! */
        n->type_ref.is_result = 1;
    }

    if (match(p, TOK_LBRACKET)) {
        /* [N]T = fixed-size, []T = dynamic */
        if (check(p, TOK_INT_LIT)) {
            char buf[32];
            int  len = p->cur.len < 31 ? p->cur.len : 31;
            memcpy(buf, p->cur.start, len); buf[len] = '\0';
            n->type_ref.arr_size = atoi(buf);
            next_tok(p);
        }
        expect(p, TOK_RBRACKET);
        n->type_ref.is_arr = 1;
    }
    if (match(p, TOK_IMU))        n->type_ref.is_imu   = 1;
    while (match(p, TOK_STAR))    n->type_ref.is_ptr++;
    if (!n->type_ref.is_ptr && match(p, TOK_CARET)) n->type_ref.is_smart = 1;

    if (check(p, TOK_LPAREN)) {
        /* tuple type: (T1, T2, ...) */
        next_tok(p);
        while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
            node_list_push(&n->type_ref.tuple, parse_type(p));
            if (!match(p, TOK_COMMA)) break;
        }
        expect(p, TOK_RPAREN);
        if (n->type_ref.tuple.count < 2)
            parse_err(p, "tuple type needs at least two elements");
        n->type_ref.name = strdup("(tuple)");
    } else if (check(p, TOK_VOID)) {
        n->type_ref.name = strdup("void");
        next_tok(p);
    } else if (check(p, TOK_BUILTIN)) {
        /* @self — instance method marker; stored as "@self" */
        Token bt = next_tok(p);
        char *nm = malloc(bt.len + 2);
        nm[0] = '@';
        memcpy(nm + 1, bt.start, bt.len);
        nm[bt.len + 1] = '\0';
        n->type_ref.name = nm;
    } else if (check(p, TOK_IDENT)) {
        n->type_ref.name = tok_dup(next_tok(p));
        if (strcmp(n->type_ref.name, "chr") == 0)
            parse_err(p, "the 'chr' type was removed — use str");
    } else {
        parse_err(p, "expected type name");
        n->type_ref.name = strdup("?");
    }
    return n;
}

static void parse_arg_list(Parser *p, NodeList *args)
{
    expect(p, TOK_LPAREN);
    skip_newlines(p);
    while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
        node_list_push(args, parse_expr_bp(p, 0));
        skip_newlines(p);
        if (!match(p, TOK_COMMA)) break;
        skip_newlines(p);
    }
    expect(p, TOK_RPAREN);
}

/* ── Pratt parser ────────────────────────────────────────────────────────── */

typedef struct { int lbp; int rbp; } OpBp;

/* Binding powers for infix/postfix operators.
   Left-associative: lbp == rbp-1.  Returns {-1,-1} if not infix. */
static OpBp infix_bp(TokenType t)
{
    switch (t) {
        case TOK_OR_KW:                              return (OpBp){ 10, 11 };
        case TOK_AND_KW:                             return (OpBp){ 20, 21 };
        case TOK_PIPE:                               return (OpBp){ 30, 31 };
        case TOK_CARET:                              return (OpBp){ 40, 41 };
        case TOK_AMP:                                return (OpBp){ 50, 51 };
        case TOK_EQEQ: case TOK_NEQ:
        case TOK_LT:   case TOK_GT:
        case TOK_LEQ:  case TOK_GEQ:                return (OpBp){ 60, 61 };
        case TOK_LSHIFT: case TOK_RSHIFT:            return (OpBp){ 70, 71 };
        case TOK_PLUS:   case TOK_MINUS:             return (OpBp){ 80, 81 };
        case TOK_STAR:   case TOK_SLASH:
        case TOK_PERCENT:                            return (OpBp){ 90, 91 };
        /* assignment — lowest, right-associative */
        case TOK_EQ:
        case TOK_PLUS_EQ: case TOK_MINUS_EQ:
        case TOK_STAR_EQ: case TOK_SLASH_EQ:
        case TOK_PERCENT_EQ:                         return (OpBp){ 5, 4 };
        /* postfix: . -> [] () — highest */
        case TOK_DOT: case TOK_ARROW:
        case TOK_LBRACKET: case TOK_LPAREN:          return (OpBp){ 110, 111 };
        /* catch: between or(10) and and(20) */
        case TOK_CATCH:                              return (OpBp){ 12, 13 };
        case TOK_NEWLINE:  case TOK_SEMICOLON:       return (OpBp){ -1, -1 };
        default:                                     return (OpBp){ -1, -1 };
    }
}

static AstNode *parse_expr_bp(Parser *p, int min_bp)
{
    int line = p->cur.line;
    AstNode *left = NULL;

    /* ── NUD: prefix / atoms ── */

    if (match(p, TOK_LPAREN)) {
        /* grouped expression — or tuple literal: (a, b, ...) */
        left = parse_expr_bp(p, 0);
        if (check(p, TOK_COMMA)) {
            AstNode *tup = ast_node_new(NODE_TUPLE_LIT, line);
            node_list_push(&tup->array_lit.elems, left);
            while (match(p, TOK_COMMA))
                node_list_push(&tup->array_lit.elems, parse_expr_bp(p, 0));
            left = tup;
        }
        expect(p, TOK_RPAREN);
    }
    else if (check(p, TOK_MINUS) || check(p, TOK_BANG) || check(p, TOK_AMP)
             || check(p, TOK_STAR) || check(p, TOK_CARET)) {
        /* unary prefix: -x  !x  &x  *p (deref)  ^p (smart deref) */
        Token op = next_tok(p);
        AstNode *n = ast_node_new(NODE_UNARY, op.line);
        n->unary.op      = op.type;
        n->unary.operand = parse_expr_bp(p, 100); /* tighter than all infix */
        left = n;
    }
    else if (check(p, TOK_STR_LIT)) {
        left = ast_node_new(NODE_STR_LIT, line);
        left->str_lit.value = tok_dup(p->cur);
        next_tok(p);
    }
    else if (check(p, TOK_CHAR_LIT)) {
        /* token excludes quotes but keeps the escape backslash */
        char v = p->cur.len > 0 ? p->cur.start[0] : '\0';
        if (v == '\\' && p->cur.len > 1) {
            switch (p->cur.start[1]) {
                case 'n':  v = '\n'; break;
                case 't':  v = '\t'; break;
                case 'r':  v = '\r'; break;
                case '0':  v = '\0'; break;
                case '\\': v = '\\'; break;
                case '\'': v = '\''; break;
                default:   v = p->cur.start[1]; break;
            }
        }
        left = ast_node_new(NODE_CHAR_LIT, line);
        left->char_lit.value = v;
        next_tok(p);
    }
    else if (check(p, TOK_INT_LIT)) {
        char buf[32];
        int  len = p->cur.len < 31 ? p->cur.len : 31;
        memcpy(buf, p->cur.start, len); buf[len] = '\0';
        left = ast_node_new(NODE_INT_LIT, line);
        if (buf[0] == '0' && (buf[1] == 'b' || buf[1] == 'B'))
            left->int_lit.value = strtoll(buf + 2, NULL, 2);
        else
            left->int_lit.value = strtoll(buf, NULL, 0);
        next_tok(p);
    }
    else if (check(p, TOK_FLOAT_LIT)) {
        char buf[32];
        int  len = p->cur.len < 31 ? p->cur.len : 31;
        memcpy(buf, p->cur.start, len); buf[len] = '\0';
        left = ast_node_new(NODE_FLOAT_LIT, line);
        left->float_lit.value = atof(buf);
        next_tok(p);
    }
    else if (check(p, TOK_TRUE) || check(p, TOK_FALSE)) {
        left = ast_node_new(NODE_BOOL_LIT, line);
        left->bool_lit.value = check(p, TOK_TRUE) ? 1 : 0;
        next_tok(p);
    }
    else if (check(p, TOK_BUILTIN)) {
        /* @name  or  @name(args) */
        left = ast_node_new(NODE_BUILTIN_CALL, line);
        left->call.name = tok_dup(next_tok(p));
        if (check(p, TOK_LPAREN))
            parse_arg_list(p, &left->call.args);
    }
    else if (check(p, TOK_IF)) {
        left = parse_if_chain(p);
    }
    else if (check(p, TOK_WHEN)) {
        left = parse_when(p);
    }
    else if (check(p, TOK_LBRACE)) {
        /* { expr, ... } array literal  or  { .f=v, ... } struct literal */
        left = parse_brace_literal(p);
    }
    else if (check(p, TOK_DOT) && p->peek.type == TOK_LBRACE) {
        /* .{.f=v, ...} anonymous struct literal */
        next_tok(p); /* consume . */
        left = parse_brace_literal(p);
        /* brace_literal sees { .f=v } and creates NODE_STRUCT_LIT with NULL type */
    }
    else if (check(p, TOK_IDENT)) {
        Token name = next_tok(p);
        /* Foo{.f=v, ...} — named struct literal.
         * Peek ensures { is followed by .field so we don't eat a loop body. */
        if (check(p, TOK_LBRACE) && p->peek.type == TOK_DOT) {
            AstNode *n = ast_node_new(NODE_STRUCT_LIT, name.line);
            n->struct_lit.type_name = tok_dup(name);
            next_tok(p); /* consume { */
            skip_newlines(p);
            while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
                expect(p, TOK_DOT);
                AstNode *fi = ast_node_new(NODE_FIELD_INIT, p->cur.line);
                fi->field_init.name = tok_dup(expect(p, TOK_IDENT));
                expect(p, TOK_EQ);
                fi->field_init.value = parse_expr_bp(p, 0);
                node_list_push(&n->struct_lit.fields, fi);
                skip_newlines(p);
                match(p, TOK_COMMA);
                skip_newlines(p);
            }
            expect(p, TOK_RBRACE);
            left = n;
        } else {
            left = ast_node_new(NODE_IDENT, name.line);
            left->ident.name = tok_dup(name);
        }
    }
    else if (check(p, TOK_TRY)) {
        /* try expr — propagate error to caller */
        next_tok(p);
        left = ast_node_new(NODE_TRY_EXPR, line);
        left->try_expr.expr = parse_expr_bp(p, 100);
    }
    else if (check(p, TOK_ERR)) {
        /* err.NAME — inline anonymous error literal */
        next_tok(p);
        left = ast_node_new(NODE_ERR_LIT, line);
        left->err_lit.set_name = NULL;
        expect(p, TOK_DOT);
        left->err_lit.variant_name = tok_dup(expect(p, TOK_IDENT));
    }
    else {
        parse_err(p, "expected expression");
        next_tok(p);
        return NULL;
    }

    /* ── LED: infix / postfix ── */

    for (;;) {
        TokenType tt  = p->cur.type;
        OpBp      bp  = infix_bp(tt);
        if (bp.lbp < min_bp) break;

        /* function call: expr(args) */
        if (tt == TOK_LPAREN) {
            next_tok(p);
            AstNode *n;
            NodeList *args;
            if (left && left->kind == NODE_IDENT) {
                /* simple call: foo(args) */
                n = ast_node_new(NODE_CALL, line);
                n->call.name = strdup(left->ident.name);
                ast_free(left);
                args = &n->call.args;
            } else {
                /* qualified call: mk.foo(args), obj.method(args) */
                n = ast_node_new(NODE_CALL_EXPR, line);
                n->call_expr.callee = left;
                args = &n->call_expr.args;
            }
            while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
                node_list_push(args, parse_expr_bp(p, 0));
                if (!match(p, TOK_COMMA)) break;
            }
            expect(p, TOK_RPAREN);
            left = n;
            continue;
        }

        /* subscript: expr[i] */
        if (tt == TOK_LBRACKET) {
            next_tok(p);
            AstNode *n = ast_node_new(NODE_SUBSCRIPT, line);
            n->subscript.target = left;
            n->subscript.index  = parse_expr_bp(p, 0);
            expect(p, TOK_RBRACKET);
            left = n;
            continue;
        }

        /* field access or deref: expr.field  or  expr.*  or  expr.^ */
        if (tt == TOK_DOT) {
            next_tok(p);
            if (match(p, TOK_STAR) || match(p, TOK_CARET)) {
                AstNode *n = ast_node_new(NODE_DEREF, line);
                n->deref.target = left;
                left = n;
            } else {
                Token fname = expect(p, TOK_IDENT);
                AstNode *n  = ast_node_new(NODE_FIELD, line);
                n->field.target = left;
                n->field.name   = tok_dup(fname);
                left = n;
            }
            continue;
        }

        /* pointer field access: expr->field */
        if (tt == TOK_ARROW) {
            next_tok(p);
            Token fname = expect(p, TOK_IDENT);
            AstNode *n  = ast_node_new(NODE_FIELD_PTR, line);
            n->field.target = left;
            n->field.name   = tok_dup(fname);
            left = n;
            continue;
        }

        /* catch: expr catch fallback | expr catch { body } | expr catch |e| { body } */
        if (tt == TOK_CATCH) {
            next_tok(p); /* consume 'catch' */
            AstNode *n = ast_node_new(NODE_CATCH_EXPR, left->line);
            n->catch_expr.expr = left;
            if (check(p, TOK_PIPE)) {
                next_tok(p); /* consume '|' */
                n->catch_expr.err_var  = tok_dup(expect(p, TOK_IDENT));
                expect(p, TOK_PIPE);
                n->catch_expr.body     = parse_block(p);
                n->catch_expr.fallback = NULL;
            } else if (check(p, TOK_LBRACE)) {
                /* capture-less block — 'catch {' always means a block, so an
                   array-literal fallback needs parens: catch ({1, 2}) */
                n->catch_expr.err_var  = NULL;
                n->catch_expr.body     = parse_block(p);
                n->catch_expr.fallback = NULL;
            } else {
                n->catch_expr.fallback = parse_expr_bp(p, 13);
                n->catch_expr.err_var  = NULL;
                n->catch_expr.body     = NULL;
            }
            left = n;
            continue;
        }

        /* binary infix operators */
        Token op    = next_tok(p);
        AstNode *rhs = parse_expr_bp(p, bp.rbp);

        /* assignment operators produce NODE_ASSIGN, not NODE_BINARY */
        if (op.type == TOK_EQ       || op.type == TOK_PLUS_EQ  ||
            op.type == TOK_MINUS_EQ || op.type == TOK_STAR_EQ  ||
            op.type == TOK_SLASH_EQ || op.type == TOK_PERCENT_EQ) {
            AstNode *n = ast_node_new(NODE_ASSIGN, op.line);
            n->assign.op  = op.type;
            n->assign.lhs = left;
            n->assign.rhs = rhs;
            left = n;
        } else {
            AstNode *n = ast_node_new(NODE_BINARY, op.line);
            n->binary.op    = op.type;
            n->binary.left  = left;
            n->binary.right = rhs;
            left = n;
        }
    }

    return left;
}

/* extern fn name(params) -> ret  — no body, declares a C function */
static AstNode *parse_extern_fn(Parser *p)
{
    AstNode *n = ast_node_new(NODE_EXTERN_FN, p->cur.line);
    expect(p, TOK_EXTERN);
    expect(p, TOK_FN);
    n->extern_fn.name = tok_dup(expect(p, TOK_IDENT));
    n->extern_fn.is_variadic = 0;

    /* params, with optional trailing ... for variadic */
    expect(p, TOK_LPAREN);
    while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
        /* ... signals variadic — consume and stop */
        if (check(p, TOK_ELLIPSIS)) {
            n->extern_fn.is_variadic = 1;
            next_tok(p);
            break;
        }
        AstNode *param = ast_node_new(NODE_PARAM, p->cur.line);
        param->param.name = tok_dup(expect(p, TOK_IDENT));
        expect(p, TOK_COLON);
        param->param.type = parse_type(p);
        node_list_push(&n->extern_fn.params, param);
        if (!match(p, TOK_COMMA)) break;
    }
    expect(p, TOK_RPAREN);

    if (match(p, TOK_ARROW)) {
        if (check(p, TOK_VOID)) next_tok(p);
        else n->extern_fn.ret_type = parse_type(p);
    }
    return n;
}

/* true if token's text equals s exactly */
static int tok_text_is(Token t, const char *s)
{
    return (int)strlen(s) == t.len && strncmp(t.start, s, t.len) == 0;
}

/* parse @import ( alias = source, ... ) and push entries into prog.imports */
static void parse_import_block(Parser *p, AstNode *prog)
{
    next_tok(p);                    /* consume '@import' */
    expect(p, TOK_LPAREN);
    skip_newlines(p);

    while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
        AstNode *entry = ast_node_new(NODE_IMPORT_DECL, p->cur.line);

        entry->import_decl.alias = tok_dup(expect(p, TOK_IDENT));
        expect(p, TOK_EQ);

        if (check(p, TOK_STR_LIT)) {
            /* "path/to/file" */
            entry->import_decl.source = tok_dup(p->cur);
            entry->import_decl.is_lib = 0;
            next_tok(p);
        } else if (check(p, TOK_BUILTIN) && tok_text_is(p->cur, "std")) {
            /* @std  or  @std.module */
            entry->import_decl.source = tok_dup(p->cur);
            entry->import_decl.is_lib = 1;
            next_tok(p);
            if (match(p, TOK_DOT))
                entry->import_decl.module = tok_dup(expect(p, TOK_IDENT));
        } else if (check(p, TOK_BUILTIN) && tok_text_is(p->cur, "pkg")) {
            /* @pkg.libname — external dep from olrn_pkg.toml */
            parse_err(p, "@pkg imports need olrn_pkg.toml support (not implemented yet)");
            ast_free(entry);
            while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) next_tok(p);
            break;
        } else {
            parse_err(p, "expected import source (\"path\", @std, or @std.module)");
            ast_free(entry);
            while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) next_tok(p);
            break;
        }

        node_list_push(&prog->program.imports, entry);
        skip_newlines(p);
        match(p, TOK_COMMA);
        skip_newlines(p);
    }
    expect(p, TOK_RPAREN);
}

/* a, b := expr  or  a, b :: expr — tuple destructuring */
static AstNode *parse_multi_bind(Parser *p)
{
    AstNode *n = ast_node_new(NODE_MULTI_BIND, p->cur.line);
    for (;;) {
        Token t = expect(p, TOK_IDENT);
        AstNode *id = ast_node_new(NODE_IDENT, t.line);
        id->ident.name = tok_dup(t);
        node_list_push(&n->multi_bind.names, id);
        if (!match(p, TOK_COMMA)) break;
    }
    if (match(p, TOK_WALRUS))      n->multi_bind.is_imu = 0;
    else if (match(p, TOK_COLCOL)) n->multi_bind.is_imu = 1;
    else parse_err(p, "expected ':=' or '::' after destructuring names");
    n->multi_bind.init = parse_expr_bp(p, 0);
    return n;
}

/* x := val  (implicit mutable)  or  x :: val  (implicit immutable) */
static AstNode *parse_var_decl_implicit(Parser *p, int is_imu)
{
    AstNode *n = ast_node_new(NODE_VAR_DECL, p->cur.line);
    n->var_decl.name    = tok_dup(next_tok(p)); /* consume name */
    n->var_decl.is_imu  = is_imu;
    n->var_decl.type_ref = NULL;                /* auto-infer */
    next_tok(p);                                /* consume := or :: */
    n->var_decl.init = parse_expr_bp(p, 0);
    return n;
}

/* x :T = val  or  x :T: val  (explicit type) */
static AstNode *parse_var_decl_explicit(Parser *p)
{
    AstNode *n = ast_node_new(NODE_VAR_DECL, p->cur.line);
    n->var_decl.name = tok_dup(next_tok(p)); /* consume name */
    next_tok(p);                             /* consume : */
    n->var_decl.type_ref = parse_type(p);
    if (match(p, TOK_EQ)) {
        n->var_decl.is_imu = 0;
    } else if (match(p, TOK_COLON)) {
        n->var_decl.is_imu = 1;
    } else {
        parse_err(p, "expected '=' or ':' after type in variable declaration");
        n->var_decl.is_imu = 0;
    }
    if (check(p, TOK_UNDEF)) {
        n->var_decl.init = NULL; /* undef = no initializer */
        next_tok(p);
    } else {
        n->var_decl.init = parse_expr_bp(p, 0);
    }
    return n;
}

/* mut T: a=v, b=v  or  imu T: a=v, b=v */
static AstNode *parse_multi_var_decl(Parser *p)
{
    AstNode *n = ast_node_new(NODE_VAR_DECL_GROUP, p->cur.line);
    n->var_decl_group.is_imu = check(p, TOK_IMU) ? 1 : 0;
    next_tok(p);                         /* consume mut or imu */
    n->var_decl_group.type_ref = parse_type(p);
    expect(p, TOK_COLON);

    do {
        AstNode *entry = ast_node_new(NODE_VAR_DECL, p->cur.line);
        entry->var_decl.name     = tok_dup(expect(p, TOK_IDENT));
        entry->var_decl.is_imu   = n->var_decl_group.is_imu;
        entry->var_decl.type_ref = NULL; /* shared from group */
        expect(p, TOK_EQ);
        entry->var_decl.init = parse_expr_bp(p, 0);
        node_list_push(&n->var_decl_group.entries, entry);
    } while (match(p, TOK_COMMA));

    return n;
}

/* while <cond> { } */
static AstNode *parse_while(Parser *p)
{
    AstNode *n = ast_node_new(NODE_WHILE, p->cur.line);
    next_tok(p); /* consume 'while' */
    n->while_loop.cond = parse_expr_bp(p, 0);
    n->while_loop.body = parse_block(p);
    return n;
}

/* loop init, cond, step { } */
static AstNode *parse_loop(Parser *p)
{
    AstNode *n = ast_node_new(NODE_LOOP, p->cur.line);
    next_tok(p); /* consume 'loop' */

    /* init — var decl or expression */
    if (check(p, TOK_IDENT) && p->peek.type == TOK_WALRUS)
        n->loop_stmt.init = parse_var_decl_implicit(p, 0);
    else if (check(p, TOK_IDENT) && p->peek.type == TOK_COLON)
        n->loop_stmt.init = parse_var_decl_explicit(p);
    else
        n->loop_stmt.init = parse_expr_bp(p, 0);
    expect(p, TOK_COMMA);

    n->loop_stmt.cond = parse_expr_bp(p, 0);
    expect(p, TOK_COMMA);

    n->loop_stmt.step = parse_expr_bp(p, 0);
    n->loop_stmt.body = parse_block(p);
    return n;
}

/* for 0..N { }  or  for e => iter { }  or  for e, i => iter { } */
static AstNode *parse_for(Parser *p)
{
    next_tok(p); /* consume 'for' */

    /* for e => iter  or  for _, i => iter  or  for e, i => iter */
    if (check(p, TOK_IDENT) && p->peek.type == TOK_FAT_ARROW) {
        AstNode *n = ast_node_new(NODE_FOR_EACH, p->cur.line);
        n->for_each.elem = tok_dup(next_tok(p));
        next_tok(p); /* consume => */
        n->for_each.idx  = NULL;
        n->for_each.iter = parse_expr_bp(p, 0);
        n->for_each.body = parse_block(p);
        return n;
    }
    if (check(p, TOK_IDENT) && p->peek.type == TOK_COMMA) {
        AstNode *n = ast_node_new(NODE_FOR_EACH, p->cur.line);
        Token elem = next_tok(p);          /* e or _ */
        n->for_each.elem = tok_dup(elem);
        next_tok(p);                        /* consume , */
        n->for_each.idx  = tok_dup(expect(p, TOK_IDENT));
        expect(p, TOK_FAT_ARROW);
        n->for_each.iter = parse_expr_bp(p, 0);
        n->for_each.body = parse_block(p);
        return n;
    }

    /* for lo..hi { } — range loop */
    AstNode *n = ast_node_new(NODE_FOR_RANGE, p->cur.line);
    n->for_range.lo = parse_expr_bp(p, 0);
    if (check(p, TOK_DOTDOTEQ)) {
        n->for_range.inclusive = 1;
        next_tok(p);
    } else {
        n->for_range.inclusive = 0;
        expect(p, TOK_DOTDOT);
    }
    n->for_range.hi   = parse_expr_bp(p, 0);
    n->for_range.body = parse_block(p);
    return n;
}

/* {e,...} array lit  or  {.f=v,...} struct lit (without a type name prefix) */
static AstNode *parse_brace_literal(Parser *p)
{
    int line = p->cur.line;
    next_tok(p); /* consume { */

    /* { .field = val, ... } → struct literal */
    if (check(p, TOK_DOT)) {
        AstNode *n = ast_node_new(NODE_STRUCT_LIT, line);
        n->struct_lit.type_name = NULL;
        while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
            skip_newlines(p);
            if (check(p, TOK_RBRACE)) break;
            expect(p, TOK_DOT);
            AstNode *fi = ast_node_new(NODE_FIELD_INIT, p->cur.line);
            fi->field_init.name = tok_dup(expect(p, TOK_IDENT));
            expect(p, TOK_EQ);
            fi->field_init.value = parse_expr_bp(p, 0);
            node_list_push(&n->struct_lit.fields, fi);
            skip_newlines(p);
            match(p, TOK_COMMA);
            skip_newlines(p);
        }
        expect(p, TOK_RBRACE);
        return n;
    }

    /* { expr, ... } → array literal */
    AstNode *n = ast_node_new(NODE_ARRAY_LIT, line);
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        node_list_push(&n->array_lit.elems, parse_expr_bp(p, 0));
        skip_newlines(p);
        if (!match(p, TOK_COMMA)) break;
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

/* type Name = T */
static AstNode *parse_type_alias(Parser *p)
{
    AstNode *n = ast_node_new(NODE_TYPE_ALIAS, p->cur.line);
    next_tok(p); /* consume 'type' */
    n->type_alias.name   = tok_dup(expect(p, TOK_IDENT));
    expect(p, TOK_EQ);
    n->type_alias.target = parse_type(p);
    return n;
}

/* struct Name { field: type, ...  pub fn method(...) { } ... } */
static AstNode *parse_struct_decl(Parser *p)
{
    AstNode *n = ast_node_new(NODE_STRUCT_DECL, p->cur.line);
    next_tok(p); /* consume 'struct' */
    n->struct_decl.name = tok_dup(expect(p, TOK_IDENT));
    expect(p, TOK_LBRACE);
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (check(p, TOK_FN)) {
            node_list_push(&n->struct_decl.methods, parse_fn_decl(p));
        } else if (check(p, TOK_PUB)) {
            next_tok(p); /* consume 'pub' */
            if (check(p, TOK_FN)) {
                /* pub fn — method */
                node_list_push(&n->struct_decl.methods, parse_fn_decl(p));
            } else {
                /* pub NAME : type = expr  — static field */
                AstNode *sv = ast_node_new(NODE_VAR_DECL, p->cur.line);
                sv->var_decl.name = tok_dup(expect(p, TOK_IDENT));
                expect(p, TOK_COLON);
                sv->var_decl.type_ref = parse_type(p);
                if (match(p, TOK_COLON)) {
                    sv->var_decl.is_imu = 1;
                } else {
                    expect(p, TOK_EQ);
                    sv->var_decl.is_imu = 0;
                }
                sv->var_decl.init = parse_expr(p);
                node_list_push(&n->struct_decl.statics, sv);
            }
        } else {
            /* field: name: type */
            AstNode *field = ast_node_new(NODE_PARAM, p->cur.line);
            field->param.name = tok_dup(expect(p, TOK_IDENT));
            expect(p, TOK_COLON);
            field->param.type = parse_type(p);
            node_list_push(&n->struct_decl.fields, field);
        }
        skip_newlines(p);
        match(p, TOK_COMMA);
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

/* enum Name { A, B }  or  enum Name => T { A=val, B=val } */
static AstNode *parse_enum_decl(Parser *p)
{
    AstNode *n = ast_node_new(NODE_ENUM_DECL, p->cur.line);
    next_tok(p); /* consume 'enum' */
    n->enum_decl.name = tok_dup(expect(p, TOK_IDENT));
    if (match(p, TOK_FAT_ARROW))
        n->enum_decl.base_type = parse_type(p);
    expect(p, TOK_LBRACE);
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *v = ast_node_new(NODE_ENUM_VARIANT, p->cur.line);
        v->enum_variant.name  = tok_dup(expect(p, TOK_IDENT));
        v->enum_variant.value = match(p, TOK_EQ) ? parse_expr_bp(p, 0) : NULL;
        node_list_push(&n->enum_decl.variants, v);
        skip_newlines(p);
        match(p, TOK_COMMA);
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

/* unn Name { a: T, b: U }  or  unn(enum) Name { ... } */
static AstNode *parse_unn_decl(Parser *p)
{
    AstNode *n = ast_node_new(NODE_UNN_DECL, p->cur.line);
    next_tok(p); /* consume 'unn' */
    if (match(p, TOK_LPAREN)) {
        /* unn(enum) — tagged union; consume the 'enum' keyword */
        next_tok(p);
        expect(p, TOK_RPAREN);
        n->unn_decl.is_tagged = 1;
    }
    n->unn_decl.name = tok_dup(expect(p, TOK_IDENT));
    expect(p, TOK_LBRACE);
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *field = ast_node_new(NODE_PARAM, p->cur.line);
        field->param.name = tok_dup(expect(p, TOK_IDENT));
        expect(p, TOK_COLON);
        field->param.type = parse_type(p);
        node_list_push(&n->unn_decl.fields, field);
        skip_newlines(p);
        match(p, TOK_COMMA);
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

/* err Name { A, B, C } — error set declaration */
static AstNode *parse_err_decl(Parser *p)
{
    AstNode *n = ast_node_new(NODE_ERR_DECL, p->cur.line);
    next_tok(p); /* consume 'err' */
    n->err_decl.name = tok_dup(expect(p, TOK_IDENT));
    expect(p, TOK_LBRACE);
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *v = ast_node_new(NODE_IDENT, p->cur.line);
        v->ident.name = tok_dup(expect(p, TOK_IDENT));
        node_list_push(&n->err_decl.variants, v);
        skip_newlines(p);
        match(p, TOK_COMMA);
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

/* defer expr  or  defer { stmt... } */
static AstNode *parse_defer(Parser *p)
{
    AstNode *n = ast_node_new(NODE_DEFER, p->cur.line);
    next_tok(p); /* consume 'defer' */
    if (check(p, TOK_LBRACE))
        n->defer_stmt.expr = parse_block(p);
    else
        n->defer_stmt.expr = parse_expr(p);
    return n;
}

/* parse_if_chain — handles if, and is called recursively for elif */
static AstNode *parse_if_chain(Parser *p)
{
    AstNode *n = ast_node_new(NODE_IF, p->cur.line);
    next_tok(p);                                  /* consume if or elif */
    n->if_expr.cond       = parse_expr_bp(p, 0);
    n->if_expr.then_block = parse_block(p);

    if (check(p, TOK_ELIF)) {
        n->if_expr.else_block = parse_if_chain(p); /* recurse for elif chain */
    } else if (match(p, TOK_ELSE)) {
        n->if_expr.else_block = parse_block(p);
    } else {
        n->if_expr.else_block = NULL;
    }
    return n;
}

/* parse a single when arm:  pattern => expr,   or   _ => expr, */
static AstNode *parse_when_arm(Parser *p)
{
    AstNode *n = ast_node_new(NODE_WHEN_ARM, p->cur.line);
    skip_newlines(p);

    /* default arm: _ => ... */
    if (check(p, TOK_IDENT) && p->cur.len == 1 && p->cur.start[0] == '_') {
        next_tok(p);
        n->when_arm.pattern = NULL;
    } else {
        n->when_arm.pattern = parse_expr_bp(p, 0);
    }

    expect(p, TOK_FAT_ARROW);

    if (check(p, TOK_LBRACE)) {
        n->when_arm.body = parse_block(p);
    } else {
        n->when_arm.body = parse_expr_bp(p, 0);
    }
    match(p, TOK_COMMA);
    return n;
}

static AstNode *parse_when(Parser *p)
{
    AstNode *n = ast_node_new(NODE_WHEN, p->cur.line);
    next_tok(p);                                  /* consume when */
    n->when_expr.subject = parse_expr_bp(p, 0);
    expect(p, TOK_LBRACE);
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        node_list_push(&n->when_expr.arms, parse_when_arm(p));
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

static AstNode *parse_stmt(Parser *p)
{
    if (check(p, TOK_RET)) {
        AstNode *n = ast_node_new(NODE_RET, p->cur.line);
        next_tok(p);
        if (!check(p, TOK_RBRACE) && !check(p, TOK_EOF))
            n->ret.value = parse_expr(p);
        match(p, TOK_SEMICOLON);
        return n;
    }
    if (check(p, TOK_IF))    return parse_if_chain(p);
    if (check(p, TOK_WHEN))  return parse_when(p);
    if (check(p, TOK_WHILE)) return parse_while(p);
    if (check(p, TOK_LOOP))  return parse_loop(p);
    if (check(p, TOK_FOR))   return parse_for(p);
    if (check(p, TOK_DEFER)) return parse_defer(p);
    if (check(p, TOK_ERRDEFER)) {
        AstNode *n = ast_node_new(NODE_ERRDEFER, p->cur.line);
        next_tok(p);
        if (check(p, TOK_LBRACE)) n->errdefer_stmt.expr = parse_block(p);
        else                       n->errdefer_stmt.expr = parse_expr(p);
        return n;
    }

    /* variable declarations */
    if (check(p, TOK_MUT) || check(p, TOK_IMU))
        return parse_multi_var_decl(p);
    if (check(p, TOK_IDENT)) {
        if (p->peek.type == TOK_WALRUS) return parse_var_decl_implicit(p, 0);
        if (p->peek.type == TOK_COLCOL) return parse_var_decl_implicit(p, 1);
        if (p->peek.type == TOK_COLON)  return parse_var_decl_explicit(p);
        if (p->peek.type == TOK_COMMA)  return parse_multi_bind(p);
    }

    AstNode *expr = parse_expr(p);
    /* consume optional statement terminator */
    while (p->cur.type == TOK_NEWLINE || p->cur.type == TOK_SEMICOLON)
        next_tok(p);
    return expr;
}

static AstNode *parse_block(Parser *p)
{
    AstNode *n = ast_node_new(NODE_BLOCK, p->cur.line);
    skip_newlines(p);
    expect(p, TOK_LBRACE);
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *s = parse_stmt(p);
        if (s) node_list_push(&n->block.stmts, s);
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

static void parse_params(Parser *p, NodeList *params)
{
    expect(p, TOK_LPAREN);
    skip_newlines(p);
    while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
        AstNode *param = ast_node_new(NODE_PARAM, p->cur.line);
        param->param.name = tok_dup(expect(p, TOK_IDENT));
        expect(p, TOK_COLON);
        param->param.type = parse_type(p);
        node_list_push(params, param);
        skip_newlines(p);
        if (!match(p, TOK_COMMA)) break;
        skip_newlines(p);
    }
    expect(p, TOK_RPAREN);
}

static AstNode *parse_fn_decl(Parser *p)
{
    AstNode *n = ast_node_new(NODE_FN_DECL, p->cur.line);
    expect(p, TOK_FN);
    n->fn_decl.name = tok_dup(expect(p, TOK_IDENT));
    parse_params(p, &n->fn_decl.params);
    skip_newlines(p);

    if (match(p, TOK_ARROW)) {
        /* void return: consume the keyword, leave ret_type NULL */
        if (check(p, TOK_VOID)) next_tok(p);
        else n->fn_decl.ret_type = parse_type(p);
        skip_newlines(p);
    }

    n->fn_decl.body = parse_block(p);
    return n;
}

/* convert a parsed `name :: @std.module` (or `name :: @std`) top-level
   decl into an import entry; returns 1 if converted, 0 to keep the decl */
static int try_module_bind(AstNode *prog, AstNode *decl)
{
    if (decl->kind != NODE_VAR_DECL || !decl->var_decl.is_imu ||
        decl->var_decl.type_ref || !decl->var_decl.init)
        return 0;

    AstNode *init = decl->var_decl.init;
    AstNode *root = init;
    const char *module = NULL;
    if (init->kind == NODE_FIELD) {
        root   = init->field.target;
        module = init->field.name;
    }
    if (root->kind != NODE_BUILTIN_CALL || root->call.args.count != 0 ||
        strcmp(root->call.name, "std") != 0)
        return 0;

    AstNode *entry = ast_node_new(NODE_IMPORT_DECL, decl->line);
    entry->import_decl.alias  = decl->var_decl.name;   /* steal */
    entry->import_decl.source = strdup("std");
    entry->import_decl.module = module ? strdup(module) : NULL;
    entry->import_decl.is_lib = 1;
    node_list_push(&prog->program.imports, entry);

    decl->var_decl.name = NULL;
    ast_free(decl);
    return 1;
}

AstNode *parser_parse_program(Parser *p)
{
    AstNode *prog = ast_node_new(NODE_PROGRAM, 1);

    /* optional @import block at the top of the file */
    if (check(p, TOK_IMPORT) ||
        (check(p, TOK_BUILTIN) && tok_text_is(p->cur, "import")))
        parse_import_block(p, prog);

    skip_newlines(p);
    while (!check(p, TOK_EOF)) {
        skip_newlines(p);
        if (check(p, TOK_EOF)) break;
        if (check(p, TOK_EXTERN)) {
            node_list_push(&prog->program.decls, parse_extern_fn(p));
        } else if (check(p, TOK_FN) || check(p, TOK_PUB)) {
            match(p, TOK_PUB);
            node_list_push(&prog->program.decls, parse_fn_decl(p));
        } else if (check(p, TOK_STRUCT)) {
            node_list_push(&prog->program.decls, parse_struct_decl(p));
        } else if (check(p, TOK_ENUM)) {
            node_list_push(&prog->program.decls, parse_enum_decl(p));
        } else if (check(p, TOK_UNN)) {
            node_list_push(&prog->program.decls, parse_unn_decl(p));
        } else if (check(p, TOK_TYPE)) {
            node_list_push(&prog->program.decls, parse_type_alias(p));
        } else if (check(p, TOK_ERR)) {
            node_list_push(&prog->program.decls, parse_err_decl(p));
        } else if (check(p, TOK_MUT) || check(p, TOK_IMU)) {
            node_list_push(&prog->program.decls, parse_multi_var_decl(p));
        } else if (check(p, TOK_IDENT) &&
                   (p->peek.type == TOK_WALRUS || p->peek.type == TOK_COLCOL ||
                    p->peek.type == TOK_COLON)) {
            /* top-level constant, variable, or module bind (io :: @std.io) */
            AstNode *decl = parse_stmt(p);
            if (!try_module_bind(prog, decl))
                node_list_push(&prog->program.decls, decl);
        } else {
            parse_err(p, "expected top-level declaration");
            next_tok(p);
        }
    }
    return prog;
}
