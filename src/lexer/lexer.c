#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void lexer_init(Lexer *l, const char *src)
{
    l->src       = src;
    l->pos       = 0;
    l->line      = 1;
    l->col       = 1;
    l->last_type = TOK_EOF; /* nothing emitted yet — won't trigger implicit newline */
    l->has_held  = 0;
}

static char cur(Lexer *l)  { return l->src[l->pos]; }
static char peek(Lexer *l) { return l->src[l->pos] ? l->src[l->pos + 1] : '\0'; }

static char adv(Lexer *l)
{
    char c = l->src[l->pos++];
    if (c == '\n') { l->line++; l->col = 1; }
    else { l->col++; }
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

static Token make_tok(int line, int col, TokenType type, const char *start, int len)
{
    Token t = {type, start, len, line, col};
    return t;
}

static struct { const char *w; TokenType t; } kw_table[] = {
    {"fn",     TOK_FN},    {"ret",    TOK_RET},   {"if",     TOK_IF},
    {"elif",   TOK_ELIF},  {"else",   TOK_ELSE},  {"for",    TOK_FOR},
    {"while",  TOK_WHILE}, {"loop",   TOK_LOOP},  {"when",   TOK_WHEN},
    {"struct", TOK_STRUCT},{"enum",   TOK_ENUM},  {"unn",    TOK_UNN},
    {"defer",  TOK_DEFER}, {"pub",    TOK_PUB},   {"void",   TOK_VOID},
    {"true",   TOK_TRUE},  {"false",  TOK_FALSE}, {"undef",  TOK_UNDEF},
    {"null",   TOK_NULL},  {"mut",    TOK_MUT},   {"imu",    TOK_IMU},
    {"static", TOK_STATIC},
    {"and",    TOK_AND_KW},{"or",     TOK_OR_KW},
    {"import",   TOK_IMPORT},
    {"extern",   TOK_EXTERN},
    {"type",     TOK_TYPE},
    {"err",      TOK_ERR},
    {"try",      TOK_TRY},
    {"catch",    TOK_CATCH},
    {"errdefer", TOK_ERRDEFER},
    {"op",       TOK_OP},
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

/* Returns 1 if a token of this type can end a statement, triggering an
   implicit newline when the source line changes afterwards. */
static int can_end_stmt(TokenType t)
{
    switch (t) {
        case TOK_IDENT:
        case TOK_INT_LIT: case TOK_FLOAT_LIT:
        case TOK_STR_LIT: case TOK_MSTR_LIT: case TOK_CHAR_LIT:
        case TOK_TRUE:    case TOK_FALSE: case TOK_NULL:
        case TOK_RPAREN:  case TOK_RBRACKET: case TOK_RBRACE:
        case TOK_DOTDEREF:
            return 1;
        default:
            return 0;
    }
}

/* Lex a single token at the current position (whitespace already skipped). */
static Token lex_one(Lexer *l)
{
    const char *start = l->src + l->pos;
    int line = l->line;
    int col  = l->col;

    if (!cur(l)) return make_tok(line, col, TOK_EOF, start, 0);

    char c = adv(l);

    /* string literal — single or triple-quoted */
    if (c == '"') {
        /* triple-quoted multiline string """...""" */
        if (cur(l) == '"' && peek(l) == '"') {
            adv(l); adv(l); /* consume 2nd and 3rd opening " */
            const char *inner = l->src + l->pos;
            while (cur(l)) {
                if (cur(l) == '"' && l->src[l->pos+1] == '"' && l->src[l->pos+2] == '"')
                    break;
                adv(l);
            }
            int inner_len = (int)(l->src + l->pos - inner);
            if (cur(l)) { adv(l); adv(l); adv(l); } /* consume closing """ */
            return make_tok(line, col, TOK_MSTR_LIT, inner, inner_len);
        }
        /* single-quoted string */
        while (cur(l) && cur(l) != '"') {
            if (cur(l) == '\\') adv(l);
            adv(l);
        }
        if (cur(l) == '"') adv(l);
        /* token value is the inner text, without quotes */
        return make_tok(line, col, TOK_STR_LIT, start + 1,
                        (int)(l->src + l->pos - start) - 2);
    }

    /* char literal */
    if (c == '\'') {
        if (cur(l) == '\\') adv(l);
        adv(l);
        if (cur(l) == '\'') adv(l);
        return make_tok(line, col, TOK_CHAR_LIT, start + 1,
                        (int)(l->src + l->pos - start) - 2);
    }

    /* builtin: @name */
    if (c == '@') {
        while (isalnum(cur(l)) || cur(l) == '_') adv(l);
        return make_tok(line, col, TOK_BUILTIN, start + 1,
                        (int)(l->src + l->pos - start) - 1);
    }

    /* identifier or keyword */
    if (isalpha(c) || c == '_') {
        while (isalnum(cur(l)) || cur(l) == '_') adv(l);
        int len = (int)(l->src + l->pos - start);
        return make_tok(line, col, check_kw(start, len), start, len);
    }

    /* number */
    if (isdigit(c)) {
        /* hex: 0x... */
        if (c == '0' && (cur(l) == 'x' || cur(l) == 'X')) {
            adv(l);
            while (isxdigit(cur(l))) adv(l);
            return make_tok(line, col, TOK_INT_LIT, start,
                            (int)(l->src + l->pos - start));
        }
        /* binary: 0b... */
        if (c == '0' && (cur(l) == 'b' || cur(l) == 'B')) {
            adv(l);
            while (cur(l) == '0' || cur(l) == '1') adv(l);
            return make_tok(line, col, TOK_INT_LIT, start,
                            (int)(l->src + l->pos - start));
        }
        while (isdigit(cur(l))) adv(l);
        if (cur(l) == '.' && isdigit(peek(l))) {
            adv(l);
            while (isdigit(cur(l))) adv(l);
            return make_tok(line, col, TOK_FLOAT_LIT, start,
                            (int)(l->src + l->pos - start));
        }
        return make_tok(line, col, TOK_INT_LIT, start,
                        (int)(l->src + l->pos - start));
    }

    /* two-char and single-char symbols */
    switch (c) {
        case '+': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_PLUS_EQ,   start,2);} return make_tok(line, col, TOK_PLUS,    start,1);
        case '=': if (cur(l)=='>'){adv(l);return make_tok(line, col, TOK_FAT_ARROW,start,2);}
                  if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_EQEQ,     start,2);} return make_tok(line, col, TOK_EQ,       start,1);
        case ':': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_WALRUS, start,2);}
                  if (cur(l)==':'){adv(l);return make_tok(line, col, TOK_COLCOL, start,2);} return make_tok(line, col, TOK_COLON,start,1);
        case '.': if (cur(l)=='.'){adv(l);
                      if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_DOTDOTEQ, start,3);}
                      if (cur(l)=='.'){adv(l);return make_tok(line, col, TOK_ELLIPSIS,  start,3);}
                      return make_tok(line, col, TOK_DOTDOT,start,2);}
                  /* .* deref postfix — consume * here so .*= doesn't become . + *= */
                  if (cur(l)=='*'){adv(l);return make_tok(line, col, TOK_DOTDEREF,start,2);}
                  return make_tok(line, col, TOK_DOT,start,1);
        case '!': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_NEQ,      start,2);} return make_tok(line, col, TOK_BANG,     start,1);
        case '<': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_LEQ,    start,2);}
                  if (cur(l)=='<'){adv(l);return make_tok(line, col, TOK_LSHIFT,start,2);} return make_tok(line, col, TOK_LT,start,1);
        case '>': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_GEQ,    start,2);}
                  if (cur(l)=='>'){adv(l);return make_tok(line, col, TOK_RSHIFT,start,2);} return make_tok(line, col, TOK_GT,start,1);
        case '(': return make_tok(line, col, TOK_LPAREN,   start, 1);
        case ')': return make_tok(line, col, TOK_RPAREN,   start, 1);
        case '{': return make_tok(line, col, TOK_LBRACE,   start, 1);
        case '}': return make_tok(line, col, TOK_RBRACE,   start, 1);
        case '[': return make_tok(line, col, TOK_LBRACKET, start, 1);
        case ']': return make_tok(line, col, TOK_RBRACKET, start, 1);
        case ',': return make_tok(line, col, TOK_COMMA,    start, 1);
        case ';': return make_tok(line, col, TOK_SEMICOLON,start, 1);
        case '*': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_STAR_EQ,   start,2);} return make_tok(line, col, TOK_STAR,   start,1);
        case '/': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_SLASH_EQ,  start,2);} return make_tok(line, col, TOK_SLASH,  start,1);
        case '%': if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_PERCENT_EQ,start,2);} return make_tok(line, col, TOK_PERCENT,start,1);
        case '-': if (cur(l)=='>'){adv(l);return make_tok(line, col, TOK_ARROW,     start,2);}
                  if (cur(l)=='='){adv(l);return make_tok(line, col, TOK_MINUS_EQ,  start,2);} return make_tok(line, col, TOK_MINUS,  start,1);
        case '&': return make_tok(line, col, TOK_AMP,      start, 1);
        case '^': return make_tok(line, col, TOK_CARET,    start, 1);
        case '|': return make_tok(line, col, TOK_PIPE,     start, 1);
    }

    return make_tok(line, col, TOK_ERROR, start, 1);
}

Token lexer_next(Lexer *l)
{
    /* return the real token that was deferred behind a TOK_NEWLINE */
    if (l->has_held) {
        Token t = l->held_tok;
        l->has_held  = 0;
        l->last_type = t.type;
        return t;
    }

    int line_before = l->line;
    skip_ws_and_comments(l);

    /* implicit newline: the source line changed after a statement-ending token */
    if (l->line > line_before && can_end_stmt(l->last_type)) {
        l->held_tok  = lex_one(l);   /* defer the real token for next call */
        l->has_held  = 1;
        l->last_type = TOK_NEWLINE;
        return make_tok(line_before, 1, TOK_NEWLINE, l->src + l->pos, 0);
    }

    Token t = lex_one(l);
    l->last_type = t.type;
    return t;
}

const char *tok_type_name(TokenType t)
{
    switch (t) {
        case TOK_INT_LIT:   return "INT_LIT";
        case TOK_FLOAT_LIT: return "FLOAT_LIT";
        case TOK_STR_LIT:   return "STR_LIT";
        case TOK_MSTR_LIT:  return "MSTR_LIT";
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
        case TOK_NULL:      return "null";
        case TOK_MUT:       return "mut";
        case TOK_IMU:       return "imu";
        case TOK_STATIC:    return "static";
        case TOK_AND_KW:    return "and";
        case TOK_OR_KW:     return "or";
        case TOK_IMPORT:    return "import";
        case TOK_EXTERN:    return "extern";
        case TOK_TYPE:      return "type";
        case TOK_ERR:       return "err";
        case TOK_TRY:       return "try";
        case TOK_CATCH:     return "catch";
        case TOK_ERRDEFER:  return "errdefer";
        case TOK_OP:        return "op";
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
        case TOK_COLCOL:    return "::";
        case TOK_DOTDOT:    return "..";
        case TOK_DOTDOTEQ:  return "..=";
        case TOK_ELLIPSIS:  return "...";
        case TOK_DOTDEREF:  return ".*";
        case TOK_PLUS_EQ:   return "+=";
        case TOK_MINUS_EQ:  return "-=";
        case TOK_STAR_EQ:   return "*=";
        case TOK_SLASH_EQ:  return "/=";
        case TOK_PERCENT_EQ:return "%=";
        case TOK_LSHIFT:    return "<<";
        case TOK_RSHIFT:    return ">>";
        case TOK_EOF:       return "EOF";
        case TOK_ERROR:     return "ERROR";
        default:            return "?";
    }
}
