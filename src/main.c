#include <stdio.h>
#include <stdlib.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: olrn <file.olrn>\n");
        return 1;
    }

    char *src = read_file(argv[1]);
    if (!src) return 1;

    Lexer lex;
    lexer_init(&lex, src);

    Parser parser;
    parser_init(&parser, &lex);

    AstNode *program = parser_parse_program(&parser);

    if (parser.had_error) {
        fprintf(stderr, "compilation failed\n");
        ast_free(program);
        free(src);
        return 1;
    }

    Codegen cg;
    codegen_init(&cg, stdout);
    codegen_emit(&cg, program);

    ast_free(program);
    free(src);
    return 0;
}
