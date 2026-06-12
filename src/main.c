#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "ast/ast.h"
#include "sema/check.h"
#include "deps/deps.h"
#include "codegen/codegen.h"

#ifdef _WIN32
#include <direct.h>
#define olrn_mkdir(p) _mkdir(p)
#else
#define olrn_mkdir(p) mkdir((p), 0755)
#endif

/* ── file helpers ─────────────────────────────────────────────── */

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

static char *resolve_path(const char *base_path, const char *rel)
{
    if (rel[0] == '/') return strdup(rel);
    const char *last_slash = strrchr(base_path, '/');
    if (!last_slash) return strdup(rel);
    size_t dir_len = (size_t)(last_slash - base_path) + 1;
    char *out = malloc(dir_len + strlen(rel) + 1);
    memcpy(out, base_path, dir_len);
    strcpy(out + dir_len, rel);
    return out;
}

static AstNode *parse_file(const char *path, char **src_out)
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

/* ── stdlib loading ───────────────────────────────────────────── */

static char *find_stdlib(void)
{
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
    if (access("stdlib", F_OK) == 0) return strdup("stdlib");
    return NULL;
}

static const char *STD_MODULES[] = {
    "io", "fs", "time", "math", "mem", "str", "log", "thread", NULL
};

/* true if any import binds the malkur module (mk = @std.malkur) */
static int imports_use_malkur(AstNode *program)
{
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib && imp->import_decl.module &&
            strcmp(imp->import_decl.module, "malkur") == 0)
            return 1;
    }
    return 0;
}

static void load_stdlib_module(AstNode *prog, const char *stdlib_path,
                               const char *lib, const char *mod,
                               char **srcs, int *count)
{
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s/%s.olrn", stdlib_path, lib, mod);
    char *src = NULL;
    AstNode *imported = parse_file(path, &src);
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

static int merge_imports(AstNode *program, const char *host_path,
                         char **extra_srcs, int *extra_count)
{
    /* any @std entry (whole-lib import or module bind) loads the stdlib once.
       malkur only loads when explicitly bound (mk = @std.malkur), since it
       drags in SDL2; it loads first so it lands after std.math in decl order
       (no fn prototypes are emitted — callees must precede callers). */
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
            if (imports_use_malkur(program))
                load_stdlib_module(program, stdlib_path, "std",
                                   "malkur", extra_srcs, extra_count);
            for (int m = 0; STD_MODULES[m]; m++)
                load_stdlib_module(program, stdlib_path, "std",
                                   STD_MODULES[m], extra_srcs, extra_count);
            free(stdlib_path);
        }
    }
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib) continue;
        char *resolved = resolve_path(host_path, imp->import_decl.source);
        char *src = NULL;
        AstNode *imported = parse_file(resolved, &src);
        free(resolved);
        if (!imported) return 0;
        NodeList tmp = program->program.decls;
        memset(&program->program.decls, 0, sizeof(NodeList));
        for (int j = 0; j < imported->program.decls.count; j++)
            node_list_push(&program->program.decls, imported->program.decls.items[j]);
        for (int j = 0; j < tmp.count; j++)
            node_list_push(&program->program.decls, tmp.items[j]);
        free(tmp.items);
        imported->program.decls.count = 0;
        ast_free(imported);
        extra_srcs[(*extra_count)++] = src;
    }
    return 1;
}

/* ── build progress tree ──────────────────────────────────────── */

/* Zig-style step tree, printed by 'olrn build' / 'olrn run' as each
   pipeline stage completes (sac/emit stay quiet). */
static int g_tree = 0;

static void tree_step(int last, const char *label, const char *detail)
{
    if (!g_tree) return;
    printf("%s %-8s %s\n", last ? "└─" : "├─", label, detail);
    fflush(stdout);
}

/* ── core compilation pipeline ────────────────────────────────── */

/* compile a generated .cpp to a binary; system-library deps (e.g.
   SDL2 for malkur) are detected from the file and resolved via
   pkg-config or platform fallbacks. Returns 0 on success. */
static int gxx_compile(const char *cpp_path, const char *output)
{
    char dep_flags[768];
    if (deps_flags_for_cpp(cpp_path, dep_flags, sizeof(dep_flags))) {
        fprintf(stderr, "build aborted — run 'olrn deps' for a full report\n");
        return 1;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "g++ -std=c++17 -O2 \"%s\" -o \"%s\"%s 2>&1", cpp_path, output,
             dep_flags);
    return (system(cmd) == 0) ? 0 : 1;
}

/* Parse olrn_path, merge imports, emit C++ to out. Returns 0 on success. */
static int compile_to_out(const char *olrn_path, FILE *out)
{
    char *src = NULL;
    AstNode *program = parse_file(olrn_path, &src);
    if (!program) return 1;
    tree_step(0, "parse", olrn_path);

    char *extra_srcs[64]; int extra_count = 0;
    if (!merge_imports(program, olrn_path, extra_srcs, &extra_count)) {
        ast_free(program); free(src); return 1;
    }
    if (g_tree) {
        char d[32];
        snprintf(d, sizeof(d), "%d module%s", extra_count,
                 extra_count == 1 ? "" : "s");
        tree_step(0, "imports", extra_count ? d : "none");
    }

    if (check_program(program)) {
        ast_free(program); free(src);
        for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);
        return 1;
    }
    tree_step(0, "sema", "ok");

    Codegen cg;
    codegen_init(&cg, out);
    codegen_emit(&cg, program);
    fflush(out);

    ast_free(program); free(src);
    for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);
    return 0;
}

/* Compile one or more .olrn files to a native binary via g++.
   inputs[0] is the main file; inputs[1..n-1] are prepended as implicit imports.
   Returns 0 on success. */
static int compile_to_binary(const char **inputs, int input_count,
                             const char *output)
{
    /* write merged C++ to a temp file */
    char tmp_cpp[64];
    snprintf(tmp_cpp, sizeof(tmp_cpp), "/tmp/olrn_%d.cpp", (int)getpid());
    FILE *cpp_file = fopen(tmp_cpp, "w");
    if (!cpp_file) { perror(tmp_cpp); return 1; }

    char *main_src = NULL;
    AstNode *program = parse_file(inputs[0], &main_src);
    if (!program) { fclose(cpp_file); remove(tmp_cpp); return 1; }
    tree_step(0, "parse", inputs[0]);

    char *extra_srcs[128]; int extra_count = 0;
    if (!merge_imports(program, inputs[0], extra_srcs, &extra_count)) {
        ast_free(program); free(main_src);
        fclose(cpp_file); remove(tmp_cpp); return 1;
    }
    if (g_tree) {
        char d[32];
        snprintf(d, sizeof(d), "%d module%s", extra_count,
                 extra_count == 1 ? "" : "s");
        tree_step(0, "imports", extra_count ? d : "none");
    }

    /* prepend any additional .olrn files as implicit imports */
    for (int i = 1; i < input_count; i++) {
        char *fsrc = NULL;
        AstNode *imported = parse_file(inputs[i], &fsrc);
        if (!imported) {
            ast_free(program); free(main_src);
            for (int j = 0; j < extra_count; j++) free(extra_srcs[j]);
            fclose(cpp_file); remove(tmp_cpp); return 1;
        }
        NodeList tmp = program->program.decls;
        memset(&program->program.decls, 0, sizeof(NodeList));
        for (int j = 0; j < imported->program.decls.count; j++)
            node_list_push(&program->program.decls, imported->program.decls.items[j]);
        for (int j = 0; j < tmp.count; j++)
            node_list_push(&program->program.decls, tmp.items[j]);
        free(tmp.items);
        imported->program.decls.count = 0;
        ast_free(imported);
        extra_srcs[extra_count++] = fsrc;
    }

    if (check_program(program)) {
        ast_free(program); free(main_src);
        for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);
        fclose(cpp_file); remove(tmp_cpp); return 1;
    }
    tree_step(0, "sema", "ok");

    Codegen cg;
    codegen_init(&cg, cpp_file);
    codegen_emit(&cg, program);
    fflush(cpp_file);
    fclose(cpp_file);
    tree_step(0, "codegen", tmp_cpp);

    ast_free(program); free(main_src);
    for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);

    tree_step(1, "g++", output);
    int rc = gxx_compile(tmp_cpp, output);
    remove(tmp_cpp);
    return rc;
}

/* ── commands ─────────────────────────────────────────────────── */

static void print_help(void)
{
    printf("olrn %s — the Oleren compiler\n\n", OLRN_VERSION);
    printf("usage:\n");
    printf("  olrn <file.olrn>               emit C++ to stdout\n");
    printf("  olrn emit <file.olrn>          emit C++ to stdout\n");
    printf("  olrn build-src <file.olrn>     emit C++ to olrn_out/<file>.cpp (project) or ./<file>.cpp\n");
    printf("  olrn build-out <file.cpp>      compile C++ to bin/<name> (project) or ./<name>\n");
    printf("  olrn check <file.olrn>         parse and check for errors\n");
    printf("  olrn sac <file(s)> [-o=name]   compile to native binary\n");
    printf("  olrn build                     build project (main.olrn → binary)\n");
    printf("  olrn run                       build and run project\n");
    printf("  olrn init                      scaffold a new project in cwd\n");
    printf("  olrn deps [file.olrn]          check system libs (SDL2, ...) + install hints\n");
    printf("  olrn version                   print version\n");
    printf("  olrn help                      print this help\n");
}

/* olrn init — scaffold in the current directory */
/* current directory name — used as the project/binary name */
static const char *project_name(void)
{
    static char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) { perror("getcwd"); return NULL; }
    const char *name = strrchr(cwd, '/');
    return name ? name + 1 : cwd;
}

#define PROJECT_MAIN "src/main/olrn/main.olrn"

/* write a file only if it doesn't exist yet; returns 0 on success */
static int write_if_missing(const char *path, const char *fmt,
                            const char *arg)
{
    if (access(path, F_OK) == 0) return 0;
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return 1; }
    fprintf(f, fmt, arg);
    fclose(f);
    printf("  %s\n", path);
    return 0;
}

static int cmd_init(int argc, char **argv)
{
    (void)argc; (void)argv;

    const char *name = project_name();
    if (!name) return 1;

    olrn_mkdir("bin");
    olrn_mkdir("src");
    olrn_mkdir("src/main");
    olrn_mkdir("src/main/olrn");
    olrn_mkdir("olrn_out");

    printf("initialized project: %s\n", name);
    if (write_if_missing(PROJECT_MAIN,
        "fn main() -> void\n"
        "{\n"
        "    @pl(\"Hello from %s!\")\n"
        "}\n", name)) return 1;
    if (write_if_missing("olrn_pkg.toml",
        "[project]\n"
        "name = \"%s\"\n"
        "version = \"0.2.0\"\n"
        "\n"
        "# external dependencies land here once the package manager exists:\n"
        "# [deps]\n", name)) return 1;
    if (write_if_missing("README.md",
        "# %s\n"
        "\n"
        "An Oleren project.\n"
        "\n"
        "```sh\n"
        "olrn build   # compile src/main/olrn/main.olrn -> bin/\n"
        "olrn run     # build then execute\n"
        "```\n", name)) return 1;
    return 0;
}

/* olrn deps [file.olrn] — report system-library deps + install hints.
   With no file, uses the project entry point; with no project, checks
   every dep the stdlib knows about (preflight mode). */
static int cmd_deps(int argc, char **argv)
{
    const char *entry = NULL;
    if (argc >= 3)                              entry = argv[2];
    else if (access(PROJECT_MAIN, F_OK) == 0)   entry = PROJECT_MAIN;
    else if (access("main.olrn", F_OK) == 0)    entry = "main.olrn";

    const SysDep *needed[16];
    int n = 0;
    if (entry) {
        char *src = NULL;
        AstNode *program = parse_file(entry, &src);
        if (!program) return 1;
        for (int i = 0; i < program->program.imports.count; i++) {
            AstNode *imp = program->program.imports.items[i];
            if (!imp->import_decl.is_lib || !imp->import_decl.module)
                continue;
            const SysDep *batch[16];
            int got = deps_for_module(imp->import_decl.module, batch, 16);
            for (int j = 0; j < got && n < 16; j++) needed[n++] = batch[j];
        }
        ast_free(program); free(src);
        printf("system deps for %s\n", entry);
        if (n == 0) { printf("└─ none required\n"); return 0; }
    } else {
        printf("system deps (no project here — checking all known)\n");
        for (int i = 0; SYS_DEPS[i].module && n < 16; i++)
            needed[n++] = &SYS_DEPS[i];
    }

    int missing = 0;
    for (int i = 0; i < n; i++) {
        const SysDep *d = needed[i];
        const char *branch = (i == n - 1) ? "└─" : "├─";
        if (dep_available(d)) {
            char ver[64];
            dep_version(d, ver, sizeof(ver));
            printf("%s %s (@std.%s)  found%s%s\n",
                   branch, d->lib, d->module, ver[0] ? " " : "", ver);
        } else {
            printf("%s %s (@std.%s)  MISSING\n", branch, d->lib, d->module);
            dep_print_hint(d);
            missing = 1;
        }
    }
    return missing;
}

/* olrn check <file.olrn> */
static int cmd_check(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn check <file.olrn>\n");
        return 1;
    }
    const char *path = argv[2];
    char *src = NULL;
    AstNode *program = parse_file(path, &src);
    if (!program) return 1;

    /* also resolve imports to surface import-level errors */
    char *extra_srcs[64]; int extra_count = 0;
    int ok = merge_imports(program, path, extra_srcs, &extra_count);
    if (ok && check_program(program)) ok = 0;

    ast_free(program); free(src);
    for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);

    if (!ok) return 1;
    printf("%s: ok\n", path);
    return 0;
}

/* olrn emit <file.olrn>  — C++ to stdout */
static int cmd_emit(int argc, char **argv)
{
    const char *path = (argc >= 3) ? argv[2] : argv[1];
    return compile_to_out(path, stdout);
}

/* olrn build-src <file.olrn>  — C++ to olrn_out/<basename>.cpp (project)
   or <basename>.cpp (flat) */
static int cmd_build_src(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn build-src <file.olrn>\n");
        return 1;
    }
    const char *in_path = argv[2];

    const char *base = strrchr(in_path, '/');
    base = base ? base + 1 : in_path;
    char stem[256];
    snprintf(stem, sizeof(stem), "%s", base);
    char *dot = strrchr(stem, '.');
    if (dot) strcpy(dot, ".cpp");
    else strcat(stem, ".cpp");

    char out_path[512];
    if (access(PROJECT_MAIN, F_OK) == 0) {
        olrn_mkdir("olrn_out");
        snprintf(out_path, sizeof(out_path), "olrn_out/%s", stem);
    } else {
        snprintf(out_path, sizeof(out_path), "%s", stem);
    }

    FILE *f = fopen(out_path, "w");
    if (!f) { perror(out_path); return 1; }
    int rc = compile_to_out(in_path, f);
    fclose(f);
    if (rc != 0) { remove(out_path); return 1; }
    printf("wrote: %s\n", out_path);
    return 0;
}

/* olrn build-out <file.cpp> [-o=name]  — compile existing C++ to binary.
   In a project, defaults to bin/<stem>; flat mode defaults to ./<stem>. */
static int cmd_build_out(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn build-out <file.cpp> [-o=name]\n");
        return 1;
    }

    const char *cpp_path = NULL;
    const char *output   = NULL;

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-o=", 3) == 0)
            output = argv[i] + 3;
        else
            cpp_path = argv[i];
    }

    if (!cpp_path) {
        fprintf(stderr, "build-out: no input file\n");
        return 1;
    }

    char derived[512];
    if (!output) {
        const char *base = strrchr(cpp_path, '/');
        base = base ? base + 1 : cpp_path;
        char stem[256];
        snprintf(stem, sizeof(stem), "%s", base);
        char *dot = strrchr(stem, '.');
        if (dot) *dot = '\0';
        if (access(PROJECT_MAIN, F_OK) == 0) {
            olrn_mkdir("bin");
            snprintf(derived, sizeof(derived), "bin/%s", stem);
        } else {
            snprintf(derived, sizeof(derived), "%s", stem);
        }
        output = derived;
    }

    int rc = gxx_compile(cpp_path, output);
    if (rc == 0) printf("built: %s\n", output);
    return rc;
}

/* olrn sac <file(s)> [-o=name] */
static int cmd_sac(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn sac <file.olrn> [more.olrn ...] [-o=name]\n");
        return 1;
    }

    const char *inputs[64];
    int input_count = 0;
    const char *output = NULL;

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-o=", 3) == 0)
            output = argv[i] + 3;
        else
            inputs[input_count++] = argv[i];
    }

    if (input_count == 0) {
        fprintf(stderr, "sac: no input files\n");
        return 1;
    }

    /* derive output name from first input if not given */
    char derived[256];
    if (!output) {
        const char *base = strrchr(inputs[0], '/');
        base = base ? base + 1 : inputs[0];
        snprintf(derived, sizeof(derived), "%s", base);
        char *dot = strrchr(derived, '.');
        if (dot) *dot = '\0';
        output = derived;
    }

    int rc = compile_to_binary(inputs, input_count, output);
    if (rc == 0) printf("built: %s\n", output);
    return rc;
}

/* olrn build — compile the project entry point.
   Project layout: src/main/olrn/main.olrn → olrn_out/<name>.cpp →
   bin/<name>. A bare main.olrn in cwd still works (flat mode) and
   builds to ./<name>. */
static int cmd_build(void)
{
    const char *name = project_name();
    if (!name) return 1;

    g_tree = 1;
    printf("%s\n", name);

    if (access(PROJECT_MAIN, F_OK) == 0) {
        olrn_mkdir("olrn_out");
        olrn_mkdir("bin");

        char cpp_path[600], bin_path[600];
        snprintf(cpp_path, sizeof(cpp_path), "olrn_out/%s.cpp", name);
        snprintf(bin_path, sizeof(bin_path), "bin/%s", name);

        FILE *out = fopen(cpp_path, "w");
        if (!out) { perror(cpp_path); return 1; }
        int rc = compile_to_out(PROJECT_MAIN, out);
        fclose(out);
        if (rc != 0) { remove(cpp_path); return 1; }
        tree_step(0, "codegen", cpp_path);

        tree_step(1, "g++", bin_path);
        rc = gxx_compile(cpp_path, bin_path);
        if (rc == 0) { printf("built: %s\n", bin_path); fflush(stdout); }
        return rc;
    }

    if (access("main.olrn", F_OK) == 0) {
        /* flat mode — single file next to you, binary in cwd */
        const char *inputs[] = { "main.olrn" };
        int rc = compile_to_binary(inputs, 1, name);
        if (rc == 0) { printf("built: %s\n", name); fflush(stdout); }
        return rc;
    }

    fprintf(stderr, "build: no %s or main.olrn found — run 'olrn init'\n",
            PROJECT_MAIN);
    return 1;
}

/* olrn run  — build then execute */
static int cmd_run(void)
{
    if (cmd_build() != 0) return 1;

    const char *name = project_name();
    if (!name) return 1;

    char cmd[768];
    if (access(PROJECT_MAIN, F_OK) == 0)
        snprintf(cmd, sizeof(cmd), "./bin/%s", name);
    else
        snprintf(cmd, sizeof(cmd), "./%s", name);
    return system(cmd);
}

/* ── entry point ──────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_help();
        return 1;
    }

    const char *sub = argv[1];

    if (strcmp(sub, "version") == 0 || strcmp(sub, "-V") == 0) {
        printf("olrn %s\n", OLRN_VERSION);
        return 0;
    }
    if (strcmp(sub, "help") == 0 || strcmp(sub, "-h") == 0) {
        print_help();
        return 0;
    }
    if (strcmp(sub, "init")      == 0) return cmd_init(argc, argv);
    if (strcmp(sub, "deps")      == 0) return cmd_deps(argc, argv);
    if (strcmp(sub, "check")     == 0) return cmd_check(argc, argv);
    if (strcmp(sub, "emit")      == 0) return cmd_emit(argc, argv);
    if (strcmp(sub, "build-src") == 0) return cmd_build_src(argc, argv);
    if (strcmp(sub, "build-out") == 0) return cmd_build_out(argc, argv);
    if (strcmp(sub, "sac")       == 0) return cmd_sac(argc, argv);
    if (strcmp(sub, "build")     == 0) return cmd_build();
    if (strcmp(sub, "run")       == 0) return cmd_run();

    if (sub[0] == '-') {
        fprintf(stderr, "unknown option '%s' — commands take no dashes "
                        "(try 'olrn help')\n", sub);
        return 1;
    }

    /* bare file path — emit C++ to stdout (backward-compatible) */
    return compile_to_out(sub, stdout);
}
