#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "ast/ast.h"
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

/* build a path relative to the directory of base_path */
static char *resolve_path(const char *base_path, const char *rel)
{
    if (rel[0] == '/') return strdup(rel); /* already absolute */

    const char *last_slash = strrchr(base_path, '/');
    if (!last_slash) return strdup(rel);

    size_t dir_len = (size_t)(last_slash - base_path) + 1;
    char *out = malloc(dir_len + strlen(rel) + 1);
    memcpy(out, base_path, dir_len);
    strcpy(out + dir_len, rel);
    return out;
}

/* parse a single .olrn file and return its AST program node */
static AstNode *parse_file(const char *path, char **src_out)
{
    char *src = read_file(path);
    if (!src) return NULL;
    *src_out = src;

    Lexer lex;
    lexer_init(&lex, src);
    Parser p;
    parser_init(&p, &lex);
    AstNode *prog = parser_parse_program(&p);
    if (p.had_error) {
        fprintf(stderr, "error in imported file: %s\n", path);
        ast_free(prog);
        free(src);
        *src_out = NULL;
        return NULL;
    }
    return prog;
}

/* find the stdlib/ directory — tries exe-relative, then cwd */
static char *find_stdlib(void)
{
    /* try executable directory first */
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n > 0) {
        exe[n] = '\0';
        char *slash = strrchr(exe, '/');
        if (slash) {
            *slash = '\0';
            char *path = malloc(strlen(exe) + 16);
            sprintf(path, "%s/stdlib", exe);
            if (access(path, F_OK) == 0) return path;
            free(path);
        }
    }
    /* fallback: current directory */
    if (access("stdlib", F_OK) == 0) return strdup("stdlib");
    return NULL;
}

/* load and prepend one stdlib .olrn module */
static void load_stdlib_module(AstNode *prog, const char *stdlib_path,
                               const char *lib, const char *mod,
                               char **srcs, int *count)
{
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s/%s.olrn", stdlib_path, lib, mod);
    char *src = NULL;
    AstNode *imported = parse_file(path, &src);
    if (!imported) return;

    /* prepend module decls before host program decls */
    NodeList tmp = prog->program.decls;
    memset(&prog->program.decls, 0, sizeof(NodeList));
    for (int j = 0; j < imported->program.decls.count; j++)
        node_list_push(&prog->program.decls, imported->program.decls.items[j]);
    for (int j = 0; j < tmp.count; j++)
        node_list_push(&prog->program.decls, tmp.items[j]);
    free(tmp.items);
    imported->program.decls.count = 0;
    ast_free(imported);
    srcs[(*count)++] = src;
}

/* stdlib module list for @libs.std */
static const char *STD_MODULES[] = {
    "time", "math", "mem", "str", "log", NULL
};

/* merge file imports: for each local file import, parse it and prepend
   its declarations to the host program (simple single-pass approach) */
static int merge_imports(AstNode *program, const char *host_path,
                         char **extra_srcs, int *extra_count)
{
    /* handle @libs.std — load all stdlib .olrn modules */
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (!imp->import_decl.is_lib) continue;
        if (strcmp(imp->import_decl.source, "std") != 0) continue;
        char *stdlib_path = find_stdlib();
        if (!stdlib_path) {
            fprintf(stderr, "warning: stdlib not found — @libs.std unavailable\n");
            continue;
        }
        for (int m = 0; STD_MODULES[m]; m++)
            load_stdlib_module(program, stdlib_path, "std",
                               STD_MODULES[m], extra_srcs, extra_count);
        free(stdlib_path);
    }

    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib) continue; /* other lib imports: codegen handles */

        char *resolved = resolve_path(host_path, imp->import_decl.source);
        char *src = NULL;
        AstNode *imported = parse_file(resolved, &src);
        free(resolved);

        if (!imported) return 0;

        /* prepend imported decls so they appear before host decls in output */
        NodeList tmp = program->program.decls;
        memset(&program->program.decls, 0, sizeof(NodeList));
        for (int j = 0; j < imported->program.decls.count; j++)
            node_list_push(&program->program.decls, imported->program.decls.items[j]);
        for (int j = 0; j < tmp.count; j++)
            node_list_push(&program->program.decls, tmp.items[j]);
        free(tmp.items);

        /* detach items so ast_free doesn't double-free them */
        imported->program.decls.count = 0;
        ast_free(imported);

        extra_srcs[(*extra_count)++] = src;
    }
    return 1;
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

    /* process local file imports */
    char *extra_srcs[64];
    int   extra_count = 0;
    if (!merge_imports(program, argv[1], extra_srcs, &extra_count)) {
        ast_free(program);
        free(src);
        return 1;
    }

    Codegen cg;
    codegen_init(&cg, stdout);
    codegen_emit(&cg, program);

    ast_free(program);
    free(src);
    for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);
    return 0;
}
