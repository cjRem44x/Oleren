#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void lexer_init(Lexer *l, const char *src)
{
    l->src  = src;
    l->pos  = 0;
    l->line = 1;
}

static char cur(Lexer *l)  { return l->src[l->pos]; }
static char peek(Lexer *l) { return l->src[l->pos] ? l->src[l->pos + 1] : '\0'; }

static char adv(Lexer *l)
{
    char c = l->src[l->pos++];
    if (c == '\n') l->line++;
    return c;
}

static void skip_ws_and_comments(Lexer *l)
{
    for (;;) {
        while (isspace(cur(l))) adv(l);
        if (cur(l) == '#') {
            while (cur(l) && cur(l) != '\n') adv(l);
            continue;
        }
        break;
    }
}

static Token make_tok(int line, TokenType type, const char *start, int len)
{
    Token t = {type, start, len, line};
    return t;
}

static struct { const char *w; TokenType t; } kw_table[] = {
    {"fn",     TOK_FN},    {"ret",    TOK_RET},   {"if",     TOK_IF},
    {"elif",   TOK_ELIF},  {"else",   TOK_ELSE},  {"for",    TOK_FOR},
    {"while",  TOK_WHILE}, {"loop",   TOK_LOOP},  {"when",   TOK_WHEN},
    {"struct", TOK_STRUCT},{"enum",   TOK_ENUM},  {"unn",    TOK_UNN},
    {"defer",  TOK_DEFER}, {"pub",    TOK_PUB},   {"void",   TOK_VOID},
    {"true",   TOK_TRUE},  {"false",  TOK_FALSE}, {"undef",  TOK_UNDEF},
    {"mut",    TOK_MUT},   {"imu",    TOK_IMU},   {"static", TOK_STATIC},
    {NULL, 0},
};

static TokenType check_kw(const char *s, int len)
{
    for (int i = 0; kw_table[i].w; i++) {
        if ((int)strlen(kw_table[i].w) == len &&
            strncmp(kw_table[i].w, s, len) == 0) {
            return kw_table[i].t;
        }
    }
    return TOK_IDENT;
}

Token lexer_next(Lexer *l)
{
    skip_ws_and_comments(l);

    const char *start = l->src + l->pos;
    int line = l->line;

    if (!cur(l)) return make_tok(line, TOK_EOF, start, 0);

    char c = adv(l);

    /* string literal */
    if (c == '"') {
        while (cur(l) && cur(l) != '"') {
            if (cur(l) == '\\') adv(l);
            adv(l);
        }
        if (cur(l) == '"') adv(l);
        /* token value is the inner text, without quotes */
        return make_tok(line, TOK_STR_LIT, start + 1,
                        (int)(l->src + l->pos - start) - 2);
    }

    /* char literal */
    if (c == '\'') {
        if (cur(l) == '\\') adv(l);
        adv(l);
        if (cur(l) == '\'') adv(l);
        return make_tok(line, TOK_CHAR_LIT, start + 1,
                        (int)(l->src + l->pos - start) - 2);
    }

    /* builtin: @name */
    if (c == '@') {
        while (isalnum(cur(l)) || cur(l) == '_') adv(l);
        return make_tok(line, TOK_BUILTIN, start + 1,
                        (int)(l->src + l->pos - start) - 1);
    }

    /* identifier or keyword */
    if (isalpha(c) || c == '_') {
        while (isalnum(cur(l)) || cur(l) == '_') adv(l);
        int len = (int)(l->src + l->pos - start);
        return make_tok(line, check_kw(start, len), start, len);
    }

    /* number */
    if (isdigit(c)) {
        while (isdigit(cur(l))) adv(l);
        if (cur(l) == '.' && isdigit(peek(l))) {
            adv(l);
            while (isdigit(cur(l))) adv(l);
            return make_tok(line, TOK_FLOAT_LIT, start,
                            (int)(l->src + l->pos - start));
        }
        return make_tok(line, TOK_INT_LIT, start,
                        (int)(l->src + l->pos - start));
    }

    /* two-char and single-char symbols */
    switch (c) {
        case '-': if (cur(l)=='>'){adv(l);return make_tok(line,TOK_ARROW,    start,2);} return make_tok(line,TOK_MINUS,    start,1);
        case '=': if (cur(l)=='>'){adv(l);return make_tok(line,TOK_FAT_ARROW,start,2);}
                  if (cur(l)=='='){adv(l);return make_tok(line,TOK_EQEQ,     start,2);} return make_tok(line,TOK_EQ,       start,1);
        case ':': if (cur(l)=='='){adv(l);return make_tok(line,TOK_WALRUS,   start,2);} return make_tok(line,TOK_COLON,    start,1);
        case '.': if (cur(l)=='.'){adv(l);
                      if (cur(l)=='='){adv(l);return make_tok(line,TOK_DOTDOTEQ,start,3);}
                      return make_tok(line,TOK_DOTDOT,start,2);}                         return make_tok(line,TOK_DOT,      start,1);
        case '!': if (cur(l)=='='){adv(l);return make_tok(line,TOK_NEQ,      start,2);} return make_tok(line,TOK_BANG,     start,1);
        case '<': if (cur(l)=='='){adv(l);return make_tok(line,TOK_LEQ,      start,2);} return make_tok(line,TOK_LT,       start,1);
        case '>': if (cur(l)=='='){adv(l);return make_tok(line,TOK_GEQ,      start,2);} return make_tok(line,TOK_GT,       start,1);
        case '(': return make_tok(line, TOK_LPAREN,   start, 1);
        case ')': return make_tok(line, TOK_RPAREN,   start, 1);
        case '{': return make_tok(line, TOK_LBRACE,   start, 1);
        case '}': return make_tok(line, TOK_RBRACE,   start, 1);
        case '[': return make_tok(line, TOK_LBRACKET, start, 1);
        case ']': return make_tok(line, TOK_RBRACKET, start, 1);
        case ',': return make_tok(line, TOK_COMMA,    start, 1);
        case ';': return make_tok(line, TOK_SEMICOLON,start, 1);
        case '+': return make_tok(line, TOK_PLUS,     start, 1);
        case '*': return make_tok(line, TOK_STAR,     start, 1);
        case '/': return make_tok(line, TOK_SLASH,    start, 1);
        case '%': return make_tok(line, TOK_PERCENT,  start, 1);
        case '&': return make_tok(line, TOK_AMP,      start, 1);
        case '^': return make_tok(line, TOK_CARET,    start, 1);
        case '|': return make_tok(line, TOK_PIPE,     start, 1);
    }

    return make_tok(line, TOK_ERROR, start, 1);
}

const char *tok_type_name(TokenType t)
{
    switch (t) {
        case TOK_INT_LIT:   return "INT_LIT";
        case TOK_FLOAT_LIT: return "FLOAT_LIT";
        case TOK_STR_LIT:   return "STR_LIT";
        case TOK_CHAR_LIT:  return "CHAR_LIT";
        case TOK_FN:        return "fn";
        case TOK_RET:       return "ret";
        case TOK_IF:        return "if";
        case TOK_ELIF:      return "elif";
        case TOK_ELSE:      return "else";
        case TOK_FOR:       return "for";
        case TOK_WHILE:     return "while";
        case TOK_LOOP:      return "loop";
        case TOK_WHEN:      return "when";
        case TOK_STRUCT:    return "struct";
        case TOK_ENUM:      return "enum";
        case TOK_UNN:       return "unn";
        case TOK_DEFER:     return "defer";
        case TOK_PUB:       return "pub";
        case TOK_VOID:      return "void";
        case TOK_TRUE:      return "true";
        case TOK_FALSE:     return "false";
        case TOK_UNDEF:     return "undef";
        case TOK_MUT:       return "mut";
        case TOK_IMU:       return "imu";
        case TOK_STATIC:    return "static";
        case TOK_IDENT:     return "IDENT";
        case TOK_BUILTIN:   return "BUILTIN";
        case TOK_LPAREN:    return "(";
        case TOK_RPAREN:    return ")";
        case TOK_LBRACE:    return "{";
        case TOK_RBRACE:    return "}";
        case TOK_LBRACKET:  return "[";
        case TOK_RBRACKET:  return "]";
        case TOK_COMMA:     return ",";
        case TOK_DOT:       return ".";
        case TOK_COLON:     return ":";
        case TOK_SEMICOLON: return ";";
        case TOK_ARROW:     return "->";
        case TOK_FAT_ARROW: return "=>";
        case TOK_PLUS:      return "+";
        case TOK_MINUS:     return "-";
        case TOK_STAR:      return "*";
        case TOK_SLASH:     return "/";
        case TOK_PERCENT:   return "%";
        case TOK_AMP:       return "&";
        case TOK_CARET:     return "^";
        case TOK_PIPE:      return "|";
        case TOK_BANG:      return "!";
        case TOK_EQ:        return "=";
        case TOK_EQEQ:      return "==";
        case TOK_NEQ:       return "!=";
        case TOK_LT:        return "<";
        case TOK_GT:        return ">";
        case TOK_LEQ:       return "<=";
        case TOK_GEQ:       return ">=";
        case TOK_WALRUS:    return ":=";
        case TOK_DOTDOT:    return "..";
        case TOK_DOTDOTEQ:  return "..=";
        case TOK_EOF:       return "EOF";
        case TOK_ERROR:     return "ERROR";
        default:            return "?";
    }
}
