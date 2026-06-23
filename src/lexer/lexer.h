#ifndef OLRN_LEXER_H
#define OLRN_LEXER_H

typedef enum {
    /* literals */
    TOK_INT_LIT, TOK_FLOAT_LIT, TOK_STR_LIT, TOK_MSTR_LIT, TOK_CHAR_LIT,

    /* keywords */
    TOK_FN, TOK_RET, TOK_IF, TOK_ELIF, TOK_ELSE,
    TOK_FOR, TOK_WHILE, TOK_LOOP, TOK_WHEN,
    TOK_STRUCT, TOK_ENUM, TOK_UNN, TOK_DEFER,
    TOK_PUB, TOK_VOID, TOK_TRUE, TOK_FALSE,
    TOK_UNDEF, TOK_NULL, TOK_MUT, TOK_IMU, TOK_STATIC,
    TOK_AND_KW, TOK_OR_KW,          /* and  or  — compile to && || */
    TOK_IMPORT,                     /* import */
    TOK_EXTERN,                     /* extern */
    TOK_TYPE,                       /* type  */
    TOK_ERR,                        /* err   */
    TOK_TRY,                        /* try   */
    TOK_CATCH,                      /* catch */
    TOK_ERRDEFER,                   /* errdefer */
    TOK_OP,                         /* op       */

    /* identifiers / builtins */
    TOK_IDENT,
    TOK_BUILTIN, /* @name */

    /* symbols */
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET,
    TOK_COMMA, TOK_DOT, TOK_COLON, TOK_SEMICOLON,
    TOK_ARROW,      /* -> */
    TOK_FAT_ARROW,  /* => */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_AMP, TOK_CARET, TOK_PIPE, TOK_BANG,
    TOK_EQ, TOK_EQEQ, TOK_NEQ,
    TOK_PLUS_EQ, TOK_MINUS_EQ,   /* += -= */
    TOK_STAR_EQ, TOK_SLASH_EQ,   /* *= /= */
    TOK_PERCENT_EQ,               /* %= */
    TOK_LT, TOK_GT, TOK_LEQ, TOK_GEQ,
    TOK_WALRUS,    /* := */
    TOK_COLCOL,    /* :: */
    TOK_DOTDOT,    /* .. */
    TOK_DOTDOTEQ,  /* ..= */
    TOK_ELLIPSIS,  /* ... */
    TOK_DOTDEREF,  /* .* raw-pointer deref postfix */
    TOK_LSHIFT,    /* << */
    TOK_RSHIFT,    /* >> */

    TOK_NEWLINE,   /* implicit statement separator — auto-inserted at line boundaries */
    TOK_EOF,
    TOK_ERROR,
} TokenType;

typedef struct {
    TokenType   type;
    const char *start; /* pointer into source buffer */
    int         len;
    int         line;
    int         col;
} Token;

typedef struct {
    const char *src;
    int         pos;
    int         line;
    int         col;
    TokenType   last_type; /* type of the last emitted token */
    int         has_held;  /* 1 = held_tok is valid and should be returned next */
    Token       held_tok;  /* real token deferred behind a TOK_NEWLINE */
} Lexer;

void        lexer_init(Lexer *l, const char *src);
Token       lexer_next(Lexer *l);
const char *tok_type_name(TokenType t);

#endif /* OLRN_LEXER_H */
