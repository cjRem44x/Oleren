#define _XOPEN_SOURCE 700
#include "resolver.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static char *canonical_path(const char *path)
{
    char *rp = realpath(path, NULL);
    return rp ? rp : strdup(path);
}

static char *resolve_path(const char *base_path, const char *rel)
{
    if (rel[0] == '/') {
        fprintf(stderr, "error: import path must be relative: '%s'\n", rel);
        return NULL;
    }
    const char *p = rel;
    while (p) {
        if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\0')) {
            fprintf(stderr, "error: import path may not contain '..': '%s'\n", rel);
            return NULL;
        }
        p = strchr(p, '/');
        if (p) p++;
    }
    const char *last_slash = strrchr(base_path, '/');
    if (!last_slash) return strdup(rel);
    size_t dir_len = (size_t)(last_slash - base_path) + 1;
    char *out = malloc(dir_len + strlen(rel) + 1);
    memcpy(out, base_path, dir_len);
    strcpy(out + dir_len, rel);
    return out;
}

AstNode *resolver_parse_file(const char *path, char **src_out)
{
    char *src = read_file(path);
    if (!src) return NULL;
    *src_out = src;
    Lexer lex; lexer_init(&lex, src);
    Parser p;  parser_init(&p, &lex);
    AstNode *prog = parser_parse_program(&p);
    if (p.had_error) {
        fprintf(stderr, "error in file: %s\n", path);
        ast_free(prog); free(src); *src_out = NULL;
        return NULL;
    }
    return prog;
}

static char *find_stdlib(void)
{
    /* 1. explicit override */
    const char *env = getenv("OLRN_STDLIB");
    if (env && access(env, F_OK) == 0) return strdup(env);

    /* 2. next to the running binary (dev build: olrn/stdlib/) */
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

    /* 3. PREFIX/lib/olrn/stdlib (deploy install: /usr/local/lib/olrn/stdlib) */
    static const char *install_paths[] = {
        "/usr/local/lib/olrn/stdlib",
        "/usr/lib/olrn/stdlib",
        NULL,
    };
    for (int i = 0; install_paths[i]; i++)
        if (access(install_paths[i], F_OK) == 0) return strdup(install_paths[i]);

    /* 4. cwd fallback */
    if (access("stdlib", F_OK) == 0) return strdup("stdlib");
    return NULL;
}

static const char *STD_MODULES[] = {
    "io", "fs", "time", "math", "mem", "str", "log", "thread", "env", "path", NULL
};

static int imports_use_module(AstNode *program, const char *name)
{
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib && imp->import_decl.module &&
            strcmp(imp->import_decl.module, name) == 0)
            return 1;
    }
    return 0;
}

static void load_stdlib_module(AstNode *prog, const char *stdlib_path,
                               const char *lib, const char *mod,
                               char **srcs, int *count, int max)
{
    if (*count >= max) {
        fprintf(stderr, "error: too many imports (limit %d)\n", max);
        return;
    }
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s/%s.olrn", stdlib_path, lib, mod);
    char *src = NULL;
    AstNode *imported = resolver_parse_file(path, &src);
    if (!imported) return;
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

/* -----------------------------------------------------------------------
   Recursive import resolution
   ----------------------------------------------------------------------- */

#define MAX_VISITED 256

typedef struct {
    AstNode *top;          /* top-level program — all MODULE nodes pushed here */
    char    *visited[MAX_VISITED]; /* canonical paths already resolved/in-progress */
    int      visited_count;
    char   **extra_srcs;
    int     *extra_count;
    int      max;
} ImportCtx;

static int ctx_seen(ImportCtx *ctx, const char *canon)
{
    for (int i = 0; i < ctx->visited_count; i++)
        if (strcmp(ctx->visited[i], canon) == 0) return 1;
    return 0;
}

static void ctx_add(ImportCtx *ctx, char *canon)
{
    if (ctx->visited_count < MAX_VISITED)
        ctx->visited[ctx->visited_count++] = canon;
    else
        free(canon);
}

/* Depth-first recursive resolution of local file imports.
   Dependencies are pushed to ctx->top->program.decls before their dependents,
   so the emitted C++ namespaces are in dependency order.
   A path added to the visited set before recursing handles both diamond deps
   (same file imported via two paths) and import cycles (silently broken). */
static int resolve_local_rec(const char *host_path, NodeList *imports,
                              ImportCtx *ctx)
{
    for (int i = 0; i < imports->count; i++) {
        AstNode *imp = imports->items[i];
        if (imp->import_decl.is_lib) continue;

        char *resolved = resolve_path(host_path, imp->import_decl.source);
        if (!resolved) return 0;

        char *canon = canonical_path(resolved);

        if (ctx_seen(ctx, canon)) {
            /* already emitted — diamond dep or import cycle; skip silently */
            free(resolved); free(canon);
            continue;
        }
        ctx_add(ctx, canon); /* mark before recursing to break cycles */

        if (*ctx->extra_count >= ctx->max) {
            fprintf(stderr, "error: too many imports (limit %d)\n", ctx->max);
            free(resolved); return 0;
        }

        char *src = NULL;
        AstNode *imported = resolver_parse_file(resolved, &src);
        if (!imported) { free(resolved); return 0; }

        /* depth-first: resolve this file's own imports before adding it */
        if (!resolve_local_rec(resolved, &imported->program.imports, ctx)) {
            free(resolved); ast_free(imported); free(src); return 0;
        }
        free(resolved);

        AstNode *mod = ast_node_new(NODE_MODULE, imp->line, imp->col);
        mod->module.name = strdup(imp->import_decl.alias); /* own the name; imp freed below */
        mod->module.decls = imported->program.decls;
        imported->program.decls.count = 0;
        imported->program.decls.items = NULL;
        ast_free(imported);
        node_list_push(&ctx->top->program.decls, mod);
        ctx->extra_srcs[(*ctx->extra_count)++] = src;
    }
    return 1;
}

int resolver_merge_imports(AstNode *program, const char *host_path,
                           char **extra_srcs, int *extra_count, int max)
{
    int wants_std = 0;
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib &&
            strcmp(imp->import_decl.source, "std") == 0)
            wants_std = 1;
    }
    if (wants_std) {
        char *stdlib_path = find_stdlib();
        if (!stdlib_path) {
            fprintf(stderr, "warning: stdlib not found — @std unavailable\n");
        } else {
            if (imports_use_module(program, "gdev"))
                load_stdlib_module(program, stdlib_path, "std",
                                   "gdev", extra_srcs, extra_count, max);
            if (imports_use_module(program, "crypt"))
                load_stdlib_module(program, stdlib_path, "std",
                                   "crypt", extra_srcs, extra_count, max);
            for (int m = 0; STD_MODULES[m]; m++)
                load_stdlib_module(program, stdlib_path, "std",
                                   STD_MODULES[m], extra_srcs, extra_count, max);
            free(stdlib_path);
        }
    }

    ImportCtx ctx = {0};
    ctx.top = program;
    ctx.extra_srcs = extra_srcs;
    ctx.extra_count = extra_count;
    ctx.max = max;

    /* add the host file itself so circular imports from transitive deps are caught */
    ctx_add(&ctx, canonical_path(host_path));

    int ok = resolve_local_rec(host_path, &program->program.imports, &ctx);

    for (int i = 0; i < ctx.visited_count; i++) free(ctx.visited[i]);
    return ok;
}
