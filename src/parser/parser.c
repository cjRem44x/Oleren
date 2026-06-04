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
static AstNode *parse_expr(Parser *p);
static AstNode *parse_block(Parser *p);

static AstNode *parse_type(Parser *p)
{
    AstNode *n = ast_node_new(NODE_TYPE_REF, p->cur.line);

    if (match(p, TOK_LBRACKET)) {
        expect(p, TOK_RBRACKET);
        n->type_ref.is_arr = 1;
    }
    if (match(p, TOK_IMU))   n->type_ref.is_imu   = 1;
    if (match(p, TOK_STAR))  n->type_ref.is_ptr   = 1;
    else if (match(p, TOK_CARET)) n->type_ref.is_smart = 1;

    if (check(p, TOK_VOID)) {
        n->type_ref.name = strdup("void");
        next_tok(p);
    } else if (check(p, TOK_IDENT)) {
        n->type_ref.name = tok_dup(next_tok(p));
    } else {
        parse_err(p, "expected type name");
        n->type_ref.name = strdup("?");
    }
    return n;
}

static void parse_arg_list(Parser *p, NodeList *args)
{
    expect(p, TOK_LPAREN);
    while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
        node_list_push(args, parse_expr(p));
        if (!match(p, TOK_COMMA)) break;
    }
    expect(p, TOK_RPAREN);
}

static AstNode *parse_expr(Parser *p)
{
    if (check(p, TOK_STR_LIT)) {
        AstNode *n = ast_node_new(NODE_STR_LIT, p->cur.line);
        n->str_lit.value = tok_dup(p->cur);
        next_tok(p);
        return n;
    }

    if (check(p, TOK_INT_LIT)) {
        AstNode *n = ast_node_new(NODE_INT_LIT, p->cur.line);
        char buf[32];
        int len = p->cur.len < 31 ? p->cur.len : 31;
        memcpy(buf, p->cur.start, len); buf[len] = '\0';
        n->int_lit.value = atoll(buf);
        next_tok(p);
        return n;
    }

    if (check(p, TOK_FLOAT_LIT)) {
        AstNode *n = ast_node_new(NODE_FLOAT_LIT, p->cur.line);
        char buf[32];
        int len = p->cur.len < 31 ? p->cur.len : 31;
        memcpy(buf, p->cur.start, len); buf[len] = '\0';
        n->float_lit.value = atof(buf);
        next_tok(p);
        return n;
    }

    if (check(p, TOK_TRUE) || check(p, TOK_FALSE)) {
        AstNode *n = ast_node_new(NODE_BOOL_LIT, p->cur.line);
        n->bool_lit.value = check(p, TOK_TRUE) ? 1 : 0;
        next_tok(p);
        return n;
    }

    /* builtin call: @name(args...) */
    if (check(p, TOK_BUILTIN)) {
        AstNode *n = ast_node_new(NODE_BUILTIN_CALL, p->cur.line);
        n->call.name = tok_dup(next_tok(p));
        parse_arg_list(p, &n->call.args);
        return n;
    }

    /* identifier or function call */
    if (check(p, TOK_IDENT)) {
        Token name = next_tok(p);
        if (check(p, TOK_LPAREN)) {
            AstNode *n = ast_node_new(NODE_CALL, name.line);
            n->call.name = tok_dup(name);
            parse_arg_list(p, &n->call.args);
            return n;
        }
        AstNode *n = ast_node_new(NODE_IDENT, name.line);
        n->ident.name = tok_dup(name);
        return n;
    }

    parse_err(p, "expected expression");
    next_tok(p);
    return NULL;
}

static AstNode *parse_stmt(Parser *p)
{
    /* ret statement */
    if (check(p, TOK_RET)) {
        AstNode *n = ast_node_new(NODE_RET, p->cur.line);
        next_tok(p);
        if (!check(p, TOK_RBRACE) && !check(p, TOK_EOF))
            n->ret.value = parse_expr(p);
        match(p, TOK_SEMICOLON);
        return n;
    }

    AstNode *expr = parse_expr(p);
    match(p, TOK_SEMICOLON);
    return expr;
}

static AstNode *parse_block(Parser *p)
{
    AstNode *n = ast_node_new(NODE_BLOCK, p->cur.line);
    expect(p, TOK_LBRACE);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *s = parse_stmt(p);
        if (s) node_list_push(&n->block.stmts, s);
    }
    expect(p, TOK_RBRACE);
    return n;
}

static void parse_params(Parser *p, NodeList *params)
{
    expect(p, TOK_LPAREN);
    while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
        AstNode *param = ast_node_new(NODE_PARAM, p->cur.line);
        param->param.name = tok_dup(expect(p, TOK_IDENT));
        expect(p, TOK_COLON);
        param->param.type = parse_type(p);
        node_list_push(params, param);
        if (!match(p, TOK_COMMA)) break;
    }
    expect(p, TOK_RPAREN);
}

static AstNode *parse_fn_decl(Parser *p)
{
    AstNode *n = ast_node_new(NODE_FN_DECL, p->cur.line);
    expect(p, TOK_FN);
    n->fn_decl.name = tok_dup(expect(p, TOK_IDENT));
    parse_params(p, &n->fn_decl.params);

    if (match(p, TOK_ARROW)) {
        /* void return: consume the keyword, leave ret_type NULL */
        if (check(p, TOK_VOID)) next_tok(p);
        else n->fn_decl.ret_type = parse_type(p);
    }

    n->fn_decl.body = parse_block(p);
    return n;
}

AstNode *parser_parse_program(Parser *p)
{
    AstNode *prog = ast_node_new(NODE_PROGRAM, 1);
    while (!check(p, TOK_EOF)) {
        if (check(p, TOK_FN) || check(p, TOK_PUB)) {
            match(p, TOK_PUB); /* consume optional pub */
            node_list_push(&prog->program.decls, parse_fn_decl(p));
        } else {
            parse_err(p, "expected top-level declaration");
            next_tok(p);
        }
    }
    return prog;
}
