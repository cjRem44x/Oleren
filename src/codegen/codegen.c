#include "codegen.h"
#include "stdlib_impl.h"
#include "malkur_impl.h"
#include "pelentar_impl.h"
#include "../lexer/lexer.h"
#include <string.h>
#include <stdio.h>

void codegen_init(Codegen *cg, FILE *out)
{
    cg->out                = out;
    cg->indent             = 0;
    cg->in_template        = 0;
    cg->type_var_count     = 0;
    cg->import_alias_count = 0;
    cg->has_stdlib         = 0;
    cg->has_malkur         = 0;
    cg->has_pelentar       = 0;
    cg->enum_count         = 0;
    cg->defer_counter      = 0;
    cg->err_count          = 0;
    cg->in_err_fn          = 0;
    cg->has_errdefer       = 0;
    cg->try_counter        = 0;
    cg->err_ret_cpp[0]     = '\0';
}

static void codegen_reset_fn_state(Codegen *cg)
{
    cg->in_template    = 0;
    cg->type_var_count = 0;
    cg->defer_counter  = 0;
    cg->in_err_fn      = 0;
    cg->has_errdefer   = 0;
    cg->try_counter    = 0;
    cg->err_ret_cpp[0] = '\0';
}

static int is_import_alias(Codegen *cg, const char *name)
{
    for (int i = 0; i < cg->import_alias_count; i++)
        if (strcmp(cg->import_aliases[i], name) == 0) return 1;
    return 0;
}

/* submodule bound to an alias (io :: @std.io → "io"), NULL if whole lib */
static const char *alias_module(Codegen *cg, const char *name)
{
    for (int i = 0; i < cg->import_alias_count; i++)
        if (strcmp(cg->import_aliases[i], name) == 0)
            return cg->import_modules[i];
    return NULL;
}

/* extract the final field name from a chain: a.b.c → "c" */
static const char *last_field_name(AstNode *node)
{
    if (node->kind == NODE_FIELD || node->kind == NODE_FIELD_PTR)
        return node->field.name;
    if (node->kind == NODE_IDENT)
        return node->ident.name;
    return NULL;
}

/* emit an import-qualified callee as a flat name: the alias root maps
   to its bound submodule (or nothing for a whole-lib alias) and the
   remaining fields join with '_', matching stdlib fn naming —
   std.io.open → io_open, and with io :: @std.io, io.open → io_open.
   (constants/types are unprefixed, so plain field access keeps the
   last-field rule: std.math.PI → PI) */
static void emit_qualified_fn(Codegen *cg, AstNode *node)
{
    AstNode *target = node->field.target;
    if (target->kind == NODE_FIELD || target->kind == NODE_FIELD_PTR) {
        emit_qualified_fn(cg, target);
        fputc('_', cg->out);
    } else if (target->kind == NODE_IDENT) {
        const char *mod = alias_module(cg, target->ident.name);
        if (mod) { fputs(mod, cg->out); fputc('_', cg->out); }
    }
    fputs(node->field.name, cg->out);
}

/* true if the root of a field chain is a known import alias */
static int callee_is_import_qualified(Codegen *cg, AstNode *callee)
{
    while (callee->kind == NODE_FIELD || callee->kind == NODE_FIELD_PTR)
        callee = callee->field.target;
    return callee->kind == NODE_IDENT && is_import_alias(cg, callee->ident.name);
}

static int is_enum_name(Codegen *cg, const char *name)
{
    for (int i = 0; i < cg->enum_count; i++)
        if (strcmp(cg->enum_names[i], name) == 0) return 1;
    return 0;
}

static int is_type_var(Codegen *cg, const char *name)
{
    for (int i = 0; i < cg->type_var_count; i++)
        if (strcmp(cg->type_vars[i], name) == 0) return 1;
    return 0;
}

static int is_err_name(Codegen *cg, const char *name)
{
    for (int i = 0; i < cg->err_count; i++)
        if (strcmp(cg->err_names[i], name) == 0) return 1;
    return 0;
}

/* returns 1 if node represents an error value (not a success value) */
static int is_err_return(Codegen *cg, AstNode *node)
{
    if (!node) return 0;
    if (node->kind == NODE_ERR_LIT) return 1;
    if (node->kind == NODE_FIELD && node->field.target->kind == NODE_IDENT)
        return is_err_name(cg, node->field.target->ident.name);
    return 0;
}

/* emit the _OlrnError value for an error return node */
static void emit_err_value(Codegen *cg, AstNode *node)
{
    if (node->kind == NODE_ERR_LIT) {
        if (node->err_lit.set_name)
            fprintf(cg->out, "_olrn_err_%s::%s",
                    node->err_lit.set_name, node->err_lit.variant_name);
        else
            fprintf(cg->out, "_OlrnError{-1, \"%s\"}", node->err_lit.variant_name);
    } else if (node->kind == NODE_FIELD) {
        fprintf(cg->out, "_olrn_err_%s::%s",
                node->field.target->ident.name, node->field.name);
    }
}

/* scan a function body for any errdefer statements */
static int fn_has_errdefer(AstNode *body)
{
    if (!body || body->kind != NODE_BLOCK) return 0;
    for (int i = 0; i < body->block.stmts.count; i++)
        if (body->block.stmts.items[i]->kind == NODE_ERRDEFER) return 1;
    return 0;
}

static void emit_indent(Codegen *cg)
{
    for (int i = 0; i < cg->indent * 4; i++) fputc(' ', cg->out);
}

/* type names that double as cast builtins: @i32(x), @str(x), etc. */
static int is_cast_builtin(const char *name)
{
    static const char *tnames[] = {
        "i8","u8","i16","u16","i32","u32","i64","u64",
        "f32","f64","str","istr","bool",
        "byte","ubyte","short","ushort","int","uint","long","ulong",
        "float","double", NULL
    };
    for (int i = 0; tnames[i]; i++)
        if (strcmp(name, tnames[i]) == 0) return 1;
    return 0;
}

/* forward declarations */
static void emit_expr(Codegen *cg, AstNode *node);
static void emit_stmt(Codegen *cg, AstNode *node);
static void emit_block_as_return(Codegen *cg, AstNode *block);
static void emit_fn_fwd(Codegen *cg, AstNode *fn);
static void emit_if_chain(Codegen *cg, AstNode *node, int continuation);
static void emit_if_expr(Codegen *cg, AstNode *node);
static void emit_when_chain(Codegen *cg, AstNode *node);
static void emit_when_expr(Codegen *cg, AstNode *node);

/* map an Oleren type name to its C++ equivalent */
static const char *map_type(const char *name)
{
    if (!name) return "void";
    if (strcmp(name, "any")  == 0) return "auto";
    if (strcmp(name, "i8")   == 0 || strcmp(name, "byte")   == 0) return "int8_t";
    if (strcmp(name, "u8")   == 0 || strcmp(name, "ubyte")  == 0) return "uint8_t";
    if (strcmp(name, "i16")  == 0 || strcmp(name, "short")  == 0) return "int16_t";
    if (strcmp(name, "u16")  == 0 || strcmp(name, "ushort") == 0) return "uint16_t";
    if (strcmp(name, "i32")  == 0 || strcmp(name, "int")    == 0) return "int32_t";
    if (strcmp(name, "u32")  == 0 || strcmp(name, "uint")   == 0) return "uint32_t";
    if (strcmp(name, "i64")  == 0 || strcmp(name, "long")   == 0) return "int64_t";
    if (strcmp(name, "u64")  == 0 || strcmp(name, "ulong")  == 0) return "uint64_t";
    if (strcmp(name, "f32")  == 0 || strcmp(name, "float")  == 0) return "float";
    if (strcmp(name, "f64")  == 0 || strcmp(name, "double") == 0) return "double";
    if (strcmp(name, "str")  == 0) return "std::string";
    if (strcmp(name, "istr") == 0) return "std::string"; /* const added in emit_type */
    if (strcmp(name, "bool") == 0) return "bool";
    if (strcmp(name, "void") == 0) return "void";
    return name; /* pass through unknown types */
}

static void emit_type(Codegen *cg, AstNode *type_ref);

/* (T1, T2, ...) → std::tuple<T1, T2, ...> */
static void emit_tuple_type(Codegen *cg, AstNode *type_ref)
{
    fputs("std::tuple<", cg->out);
    for (int i = 0; i < type_ref->type_ref.tuple.count; i++) {
        if (i > 0) fputs(", ", cg->out);
        emit_type(cg, type_ref->type_ref.tuple.items[i]);
    }
    fputc('>', cg->out);
}

static void emit_type(Codegen *cg, AstNode *type_ref)
{
    if (!type_ref) { fputs("void", cg->out); return; }

    const char *base  = map_type(type_ref->type_ref.name);
    int is_result = type_ref->type_ref.is_result;
    int is_arr    = type_ref->type_ref.is_arr;
    int arr_size  = type_ref->type_ref.arr_size;
    int is_imu    = type_ref->type_ref.is_imu;
    int is_ptr    = type_ref->type_ref.is_ptr;
    int is_smart  = type_ref->type_ref.is_smart;

    /* istr — str with immutable contents (const std::string).
       Inside !T results const is dropped: _OlrnResult needs assignable T. */
    if (type_ref->type_ref.name && !is_result &&
        strcmp(type_ref->type_ref.name, "istr") == 0)
        is_imu = 1;

    int is_tuple = type_ref->type_ref.tuple.count > 0;

    /* !T — emit as _OlrnResult<T> */
    if (is_result) {
        fputs("_OlrnResult<", cg->out);
        if (is_tuple)
            emit_tuple_type(cg, type_ref);
        else if (is_arr && arr_size > 0)
            fprintf(cg->out, "std::array<%s, %d>", base, arr_size);
        else if (is_arr)
            fprintf(cg->out, "std::vector<%s>", base);
        else if (is_smart)
            fprintf(cg->out, "std::shared_ptr<%s>", base);
        else {
            fputs(base, cg->out);
            if (is_ptr) fputs(" *", cg->out);
        }
        fputc('>', cg->out);
        return;
    }

    if (is_imu) fputs("const ", cg->out);

    if (is_tuple) {
        emit_tuple_type(cg, type_ref);
    } else if (is_arr && arr_size > 0) {
        /* [N]T — fixed-size: std::array<T, N> */
        fprintf(cg->out, "std::array<%s, %d>", base, arr_size);
    } else if (is_arr) {
        /* []T — dynamic: std::vector<T> */
        fprintf(cg->out, "std::vector<%s>", base);
    } else if (is_smart) {
        fprintf(cg->out, "std::shared_ptr<%s>", base);
    } else {
        fputs(base, cg->out);
        if (is_ptr) fputs(" *", cg->out);
    }
}

/* Returns 1 if this node is a string-concat chain (leftmost leaf is a string
   literal). Only those chains should be flattened into << segments; numeric
   `a + b` must remain as arithmetic. */
static int is_str_chain(AstNode *node)
{
    if (node->kind == NODE_STR_LIT) return 1;
    if (node->kind == NODE_BINARY && node->binary.op == TOK_PLUS)
        return is_str_chain(node->binary.left);
    return 0;
}

/* recursively flatten "str + b + c" into << str << b << c for @pl/@cout.
   Arithmetic + expressions (no leading string literal) are emitted as one operand. */
static void emit_pl_chain(Codegen *cg, AstNode *node)
{
    if (node->kind == NODE_BINARY && node->binary.op == TOK_PLUS && is_str_chain(node)) {
        emit_pl_chain(cg, node->binary.left);
        emit_pl_chain(cg, node->binary.right);
    } else {
        fputs(" << ", cg->out);
        emit_expr(cg, node);
    }
}

/* emit a single builtin call as a statement */
static void emit_builtin_stmt(Codegen *cg, AstNode *node)
{
    const char *name = node->call.name;
    emit_indent(cg);

    /* @pl(val) => std::cout << val << std::endl;
       + expressions are flattened into << chains so mixed-type concat works */
    if (strcmp(name, "pl") == 0) {
        fputs("std::cout", cg->out);
        for (int i = 0; i < node->call.args.count; i++)
            emit_pl_chain(cg, node->call.args.items[i]);
        fputs(" << std::endl;\n", cg->out);
        return;
    }

    /* @pf("Hello {name}, age {}\n", expr)
       {name}  — emit named variable verbatim (must be a simple in-scope name)
       {}      — emit next positional argument through emit_expr (supports
                 any Oleren expression: p.*, p->field, fn(), etc.)
       Both forms can be mixed in one format string. */
    if (strcmp(name, "pf") == 0 && node->call.args.count >= 1
            && node->call.args.items[0]->kind == NODE_STR_LIT) {
        const char *fmt = node->call.args.items[0]->str_lit.value;
        fputs("std::cout", cg->out);
        const char *seg = fmt, *p = fmt;
        int next_arg = 1; /* index into call.args for positional {} slots */
        while (*p) {
            if (*p == '{') {
                const char *id = p + 1;
                const char *e  = id;
                while (*e && *e != '}') e++;
                if (*e == '}') {
                    /* emit literal segment before { */
                    if (p > seg) {
                        fputs(" << \"", cg->out);
                        fwrite(seg, 1, (size_t)(p - seg), cg->out);
                        fputc('"', cg->out);
                    }
                    fputs(" << ", cg->out);
                    if (e == id) {
                        /* {} — positional: emit next arg through emit_expr */
                        if (next_arg < node->call.args.count)
                            emit_expr(cg, node->call.args.items[next_arg++]);
                        else
                            fputs("/* missing arg */", cg->out);
                    } else {
                        size_t ilen = (size_t)(e - id);
                        /* {foo.*} — Oleren deref syntax; emit as (*foo) */
                        if (ilen > 2 && e[-1] == '*' && e[-2] == '.') {
                            fputs("(*", cg->out);
                            fwrite(id, 1, ilen - 2, cg->out);
                            fputc(')', cg->out);
                        } else {
                            /* {name} or {ptr->field} — emit verbatim (valid C++) */
                            fwrite(id, 1, ilen, cg->out);
                        }
                    }
                    p   = e + 1;
                    seg = p;
                    continue;
                }
            }
            p++;
        }
        /* emit any trailing literal */
        if (p > seg) {
            fputs(" << \"", cg->out);
            fwrite(seg, 1, (size_t)(p - seg), cg->out);
            fputc('"', cg->out);
        }
        fputs(";\n", cg->out);
        return;
    }

    /* @free(p) => free(p); / delete for smart ptrs (simplified) */
    if (strcmp(name, "free") == 0) {
        fputs("free(", cg->out);
        for (int i = 0; i < node->call.args.count; i++) {
            if (i > 0) fputs(", ", cg->out);
            emit_expr(cg, node->call.args.items[i]);
        }
        fputs(");\n", cg->out);
        return;
    }

    /* @panic — print message and abort */
    if (strcmp(name, "panic") == 0) {
        emit_indent(cg);
        fputs("{ std::cerr << \"panic: \"", cg->out);
        if (node->call.args.count > 0) {
            fputs(" << ", cg->out);
            emit_expr(cg, node->call.args.items[0]);
        }
        fputs(" << std::endl; std::abort(); }\n", cg->out);
        return;
    }

    /* @assert(cond, msg) */
    if (strcmp(name, "assert") == 0 && node->call.args.count >= 1) {
        emit_indent(cg);
        fputs("if (!(", cg->out);
        emit_expr(cg, node->call.args.items[0]);
        fputs(")) { std::cerr << \"assert failed", cg->out);
        if (node->call.args.count >= 2) {
            fputs(": \" << ", cg->out);
            emit_expr(cg, node->call.args.items[1]);
            fputs(" << std::endl", cg->out);
        } else {
            fputs("\" << std::endl", cg->out);
        }
        fputs("; std::abort(); }\n", cg->out);
        return;
    }

    /* @memcpy / @memset */
    if (strcmp(name, "memcpy") == 0 && node->call.args.count == 3) {
        fputs("std::memcpy(", cg->out);
        for (int i = 0; i < 3; i++) { if (i > 0) fputs(", ", cg->out); emit_expr(cg, node->call.args.items[i]); }
        fputs(");\n", cg->out);
        return;
    }
    if (strcmp(name, "memset") == 0 && node->call.args.count == 3) {
        fputs("std::memset(", cg->out);
        for (int i = 0; i < 3; i++) { if (i > 0) fputs(", ", cg->out); emit_expr(cg, node->call.args.items[i]); }
        fputs(");\n", cg->out);
        return;
    }
    if (strcmp(name, "unreachable") == 0) {
        fputs("__builtin_unreachable();\n", cg->out);
        return;
    }

    /* fallback: emit as a plain call */
    fprintf(cg->out, "/* @%s */ (", name);
    for (int i = 0; i < node->call.args.count; i++) {
        if (i > 0) fputs(", ", cg->out);
        emit_expr(cg, node->call.args.items[i]);
    }
    fputs(");\n", cg->out);
}

static void emit_expr(Codegen *cg, AstNode *node)
{
    if (!node) return;
    switch (node->kind) {
        case NODE_STR_LIT:
            fprintf(cg->out, "\"%s\"", node->str_lit.value);
            break;
        case NODE_INT_LIT:
            /* Oleren: untyped integer literals always infer to i64 */
            fprintf(cg->out, "(int64_t)%lld", node->int_lit.value);
            break;
        case NODE_FLOAT_LIT: {
            /* Oleren: float literals are f64 (double); always include decimal
               point so C++ doesn't deduce an int type in template contexts */
            char _fb[32];
            snprintf(_fb, sizeof(_fb), "%g", node->float_lit.value);
            fputs(_fb, cg->out);
            if (!strchr(_fb, '.') && !strchr(_fb, 'e') && !strchr(_fb, 'E'))
                fputs(".0", cg->out);
            break;
        }
        case NODE_CHAR_LIT:
            switch (node->char_lit.value) {
                case '\n': fputs("'\\n'",  cg->out); break;
                case '\t': fputs("'\\t'",  cg->out); break;
                case '\r': fputs("'\\r'",  cg->out); break;
                case '\0': fputs("'\\0'",  cg->out); break;
                case '\\': fputs("'\\\\'", cg->out); break;
                case '\'': fputs("'\\''",  cg->out); break;
                default:   fprintf(cg->out, "'%c'", node->char_lit.value);
            }
            break;
        case NODE_BOOL_LIT:
            fputs(node->bool_lit.value ? "true" : "false", cg->out);
            break;
        case NODE_IDENT:
            fputs(node->ident.name, cg->out);
            break;
        case NODE_CALL:
            fprintf(cg->out, "%s(", node->call.name);
            for (int i = 0; i < node->call.args.count; i++) {
                if (i > 0) fputs(", ", cg->out);
                emit_expr(cg, node->call.args.items[i]);
            }
            fputc(')', cg->out);
            break;
        case NODE_CALL_EXPR: {
            AstNode *ce = node->call_expr.callee;
            /* .len() — treat as property, callee emit already includes .size() */
            if (ce->kind == NODE_FIELD && strcmp(ce->field.name, "len") == 0 &&
                node->call_expr.args.count == 0) {
                emit_expr(cg, ce);
                break;
            }
            /* strip import alias prefix: utils.square(x) → square(x),
               std.io.open(f) → io_open(f) */
            if (callee_is_import_qualified(cg, ce) &&
                (ce->kind == NODE_FIELD || ce->kind == NODE_FIELD_PTR)) {
                emit_qualified_fn(cg, ce);
            } else {
                emit_expr(cg, ce);
            }
            fputc('(', cg->out);
            for (int i = 0; i < node->call_expr.args.count; i++) {
                if (i > 0) fputs(", ", cg->out);
                emit_expr(cg, node->call_expr.args.items[i]);
            }
            fputc(')', cg->out);
            break;
        }
        case NODE_BUILTIN_CALL: {
            const char *bname = node->call.name;
            if (node->call.args.count == 0) {
                if      (strcmp(bname, "cout") == 0)       fputs("std::cout", cg->out);
                else if (strcmp(bname, "endl") == 0)       fputs("std::endl", cg->out);
                else if (strcmp(bname, "unreachable") == 0) fputs("(__builtin_unreachable(), 0)", cg->out);
                else    fprintf(cg->out, "/* @%s */", bname);
            } else if (is_cast_builtin(bname) && node->call.args.count == 1) {
                AstNode *arg = node->call.args.items[0];
                if (strcmp(bname, "str") == 0 || strcmp(bname, "istr") == 0) {
                    fputs("std::to_string(", cg->out); emit_expr(cg, arg); fputc(')', cg->out);
                } else if (strcmp(bname, "bool") == 0) {
                    fputs("(bool)(", cg->out); emit_expr(cg, arg); fputc(')', cg->out);
                } else {
                    /* _olrn_cast: static_cast for numerics, fallible
                       _OlrnResult<T> parse for string args */
                    fprintf(cg->out, "_olrn_cast<%s>(", map_type(bname));
                    emit_expr(cg, arg); fputc(')', cg->out);
                }
            } else if (strcmp(bname, "alo") == 0 && node->call.args.count == 1) {
                AstNode *arg = node->call.args.items[0];
                const char *t = (arg->kind == NODE_IDENT) ? map_type(arg->ident.name) : "void";
                fprintf(cg->out, "((%s*)malloc(sizeof(%s)))", t, t);
            } else if (strcmp(bname, "sizeof") == 0 && node->call.args.count == 1) {
                AstNode *arg = node->call.args.items[0];
                const char *t = (arg->kind == NODE_IDENT) ? map_type(arg->ident.name) : "void";
                fprintf(cg->out, "(int64_t)sizeof(%s)", t);
            } else if (strcmp(bname, "alignof") == 0 && node->call.args.count == 1) {
                AstNode *arg = node->call.args.items[0];
                const char *t = (arg->kind == NODE_IDENT) ? map_type(arg->ident.name) : "void";
                fprintf(cg->out, "(int64_t)alignof(%s)", t);
            } else if (strcmp(bname, "rng") == 0 && node->call.args.count == 3) {
                AstNode *targ = node->call.args.items[0];
                const char *t = (targ->kind == NODE_IDENT) ? map_type(targ->ident.name) : "int64_t";
                fprintf(cg->out, "_olrn_rng<%s>(", t);
                emit_expr(cg, node->call.args.items[1]); fputs(", ", cg->out);
                emit_expr(cg, node->call.args.items[2]); fputc(')', cg->out);
            } else if ((strcmp(bname, "min") == 0 || strcmp(bname, "max") == 0) &&
                       node->call.args.count == 2) {
                fprintf(cg->out, "std::%s(", bname);
                emit_expr(cg, node->call.args.items[0]); fputs(", ", cg->out);
                emit_expr(cg, node->call.args.items[1]); fputc(')', cg->out);
            } else if (strcmp(bname, "clamp") == 0 && node->call.args.count == 3) {
                fputs("std::clamp(", cg->out);
                for (int i = 0; i < 3; i++) {
                    if (i > 0) fputs(", ", cg->out);
                    emit_expr(cg, node->call.args.items[i]);
                }
                fputc(')', cg->out);
            } else if (strcmp(bname, "sqrt") == 0 && node->call.args.count == 1) {
                fputs("std::sqrt(", cg->out);
                emit_expr(cg, node->call.args.items[0]); fputc(')', cg->out);
            } else if (strcmp(bname, "abs") == 0 && node->call.args.count == 1) {
                fputs("std::abs(", cg->out);
                emit_expr(cg, node->call.args.items[0]); fputc(')', cg->out);
            } else if (strcmp(bname, "cin") == 0) {
                fputs("_olrn_cin(", cg->out);
                if (node->call.args.count > 0) emit_expr(cg, node->call.args.items[0]);
                else fputs("\"\"", cg->out);
                fputc(')', cg->out);
            } else if (strcmp(bname, "type") == 0 && node->call.args.count == 1) {
                /* @type(x) in expression context → demangled type name string */
                fputs("_olrn_type_name(", cg->out);
                emit_expr(cg, node->call.args.items[0]);
                fputc(')', cg->out);
            } else {
                emit_builtin_stmt(cg, node);
            }
            break;
        }
        case NODE_BINARY: {
            /* wrap in parens so precedence is explicit in emitted C++ */
            fputc('(', cg->out);
            emit_expr(cg, node->binary.left);
            switch (node->binary.op) {
                case TOK_PLUS:    fputs(" + ",  cg->out); break;
                case TOK_MINUS:   fputs(" - ",  cg->out); break;
                case TOK_STAR:    fputs(" * ",  cg->out); break;
                case TOK_SLASH:   fputs(" / ",  cg->out); break;
                case TOK_PERCENT: fputs(" % ",  cg->out); break;
                case TOK_EQEQ:   fputs(" == ", cg->out); break;
                case TOK_NEQ:    fputs(" != ", cg->out); break;
                case TOK_LT:     fputs(" < ",  cg->out); break;
                case TOK_GT:     fputs(" > ",  cg->out); break;
                case TOK_LEQ:    fputs(" <= ", cg->out); break;
                case TOK_GEQ:    fputs(" >= ", cg->out); break;
                case TOK_AND_KW: fputs(" && ", cg->out); break;
                case TOK_OR_KW:  fputs(" || ", cg->out); break;
                case TOK_AMP:    fputs(" & ",  cg->out); break;
                case TOK_PIPE:   fputs(" | ",  cg->out); break;
                case TOK_CARET:  fputs(" ^ ",  cg->out); break;
                case TOK_LSHIFT: fputs(" << ", cg->out); break;
                case TOK_RSHIFT: fputs(" >> ", cg->out); break;
                default: fputc('?', cg->out); break;
            }
            emit_expr(cg, node->binary.right);
            fputc(')', cg->out);
            break;
        }
        case NODE_UNARY:
            switch (node->unary.op) {
                case TOK_MINUS: fputc('-',  cg->out); break;
                case TOK_BANG:  fputc('!',  cg->out); break;
                case TOK_AMP:   fputc('&',  cg->out); break;
                case TOK_STAR:  fputc('*',  cg->out); break;  /* *p  — raw deref   */
                case TOK_CARET: fputs("*",  cg->out); break;  /* ^p  — smart deref */
                default: break;
            }
            emit_expr(cg, node->unary.operand);
            break;
        case NODE_FIELD:
            /* strip import alias prefix: std.math.PI → PI;
               enums/err sets keep their namespace: mk.keys.SPACE → keys::SPACE */
            if (callee_is_import_qualified(cg, node)) {
                AstNode *t = node->field.target;
                if (t->kind == NODE_FIELD && is_enum_name(cg, t->field.name))
                    fprintf(cg->out, "%s::%s", t->field.name, node->field.name);
                else if (t->kind == NODE_FIELD && is_err_name(cg, t->field.name))
                    fprintf(cg->out, "_olrn_err_%s::%s",
                            t->field.name, node->field.name);
                else
                    fputs(last_field_name(node), cg->out);
            }
            /* Color.Red → Color::Red for enum types */
            else if (node->field.target->kind == NODE_IDENT &&
                     is_enum_name(cg, node->field.target->ident.name))
                fprintf(cg->out, "%s::%s",
                        node->field.target->ident.name, node->field.name);
            /* ErrSet.Variant → _olrn_err_ErrSet::Variant for error sets */
            else if (node->field.target->kind == NODE_IDENT &&
                     is_err_name(cg, node->field.target->ident.name))
                fprintf(cg->out, "_olrn_err_%s::%s",
                        node->field.target->ident.name, node->field.name);
            /* .len property — i64 length of str / arrays ('len' is a
               reserved field name, enforced in sema) */
            else if (strcmp(node->field.name, "len") == 0) {
                fputs("(int64_t)", cg->out);
                if (node->field.target->kind == NODE_STR_LIT) {
                    fputs("std::string(", cg->out);
                    emit_expr(cg, node->field.target);
                    fputc(')', cg->out);
                } else
                    emit_expr(cg, node->field.target);
                fputs(".size()", cg->out);
            }
            else {
                emit_expr(cg, node->field.target);
                fprintf(cg->out, ".%s", node->field.name);
            }
            break;
        case NODE_FIELD_PTR:
            emit_expr(cg, node->field.target);
            fprintf(cg->out, "->%s", node->field.name);
            break;
        case NODE_SUBSCRIPT:
            emit_expr(cg, node->subscript.target);
            fputc('[', cg->out);
            emit_expr(cg, node->subscript.index);
            fputc(']', cg->out);
            break;
        case NODE_DEREF:
            /* Oleren p.* == C++ (*p) */
            fputs("(*", cg->out);
            emit_expr(cg, node->deref.target);
            fputc(')', cg->out);
            break;
        case NODE_TUPLE_LIT:
            fputs("std::make_tuple(", cg->out);
            for (int i = 0; i < node->array_lit.elems.count; i++) {
                if (i > 0) fputs(", ", cg->out);
                emit_expr(cg, node->array_lit.elems.items[i]);
            }
            fputc(')', cg->out);
            break;
        case NODE_IF:
            emit_if_expr(cg, node);
            break;
        case NODE_WHEN:
            emit_when_expr(cg, node);
            break;
        case NODE_ARRAY_LIT:
            fputc('{', cg->out);
            for (int i = 0; i < node->array_lit.elems.count; i++) {
                if (i > 0) fputs(", ", cg->out);
                emit_expr(cg, node->array_lit.elems.items[i]);
            }
            fputc('}', cg->out);
            break;
        case NODE_STRUCT_LIT:
            if (node->struct_lit.type_name)
                fputs(node->struct_lit.type_name, cg->out);
            fputc('{', cg->out);
            for (int i = 0; i < node->struct_lit.fields.count; i++) {
                if (i > 0) fputs(", ", cg->out);
                AstNode *fi = node->struct_lit.fields.items[i];
                fprintf(cg->out, ".%s = ", fi->field_init.name);
                emit_expr(cg, fi->field_init.value);
            }
            fputc('}', cg->out);
            break;
        case NODE_ASSIGN:
            emit_expr(cg, node->assign.lhs);
            switch (node->assign.op) {
                case TOK_EQ:         fputs(" = ",   cg->out); break;
                case TOK_PLUS_EQ:    fputs(" += ",  cg->out); break;
                case TOK_MINUS_EQ:   fputs(" -= ",  cg->out); break;
                case TOK_STAR_EQ:    fputs(" *= ",  cg->out); break;
                case TOK_SLASH_EQ:   fputs(" /= ",  cg->out); break;
                case TOK_PERCENT_EQ: fputs(" %= ",  cg->out); break;
                default: break;
            }
            emit_expr(cg, node->assign.rhs);
            break;
        case NODE_ERR_LIT:
            if (node->err_lit.set_name)
                fprintf(cg->out, "_olrn_err_%s::%s",
                        node->err_lit.set_name, node->err_lit.variant_name);
            else
                fprintf(cg->out, "_OlrnError{-1, \"%s\"}", node->err_lit.variant_name);
            break;
        case NODE_TRY_EXPR: {
            /* try expr — propagate error; emit as GCC statement expression */
            int n = cg->try_counter++;
            fprintf(cg->out, "(__extension__({auto _r_%d=(", n);
            emit_expr(cg, node->try_expr.expr);
            fprintf(cg->out,
                    ");if(!_r_%d.is_ok())return _OlrnResult<%s>(_r_%d.error());_r_%d.value();}))",
                    n, cg->err_ret_cpp, n, n);
            break;
        }
        case NODE_CATCH_EXPR: {
            int n = cg->try_counter++;
            if (node->catch_expr.body) {
                /* expr catch [|e|] { body } — body must bail/ret on error path */
                fprintf(cg->out, "(__extension__({auto _r_%d=(", n);
                emit_expr(cg, node->catch_expr.expr);
                fprintf(cg->out, ");if(!_r_%d.is_ok()){", n);
                if (node->catch_expr.err_var)
                    fprintf(cg->out, "_OlrnError %s=_r_%d.error();",
                            node->catch_expr.err_var, n);
                AstNode *bdy = node->catch_expr.body;
                if (bdy->kind == NODE_BLOCK) {
                    int saved = cg->indent;
                    cg->indent = 0;
                    for (int j = 0; j < bdy->block.stmts.count; j++)
                        emit_stmt(cg, bdy->block.stmts.items[j]);
                    cg->indent = saved;
                }
                fprintf(cg->out, "}_r_%d.value();}))", n);
            } else {
                /* expr catch fallback — ternary */
                fprintf(cg->out, "(__extension__({auto _r_%d=(", n);
                emit_expr(cg, node->catch_expr.expr);
                fprintf(cg->out, ");_r_%d.is_ok()?_r_%d.value():(", n, n);
                emit_expr(cg, node->catch_expr.fallback);
                fputs(");}))", cg->out);
            }
            break;
        }
        default: break;
    }
}

/* emit the init clause of a 'loop' — no indent or newline */
static void emit_for_init(Codegen *cg, AstNode *node)
{
    if (!node) return;
    if (node->kind == NODE_VAR_DECL) {
        if (node->var_decl.is_imu) fputs("const ", cg->out);
        if (node->var_decl.type_ref) emit_type(cg, node->var_decl.type_ref);
        else                         fputs("auto", cg->out);
        fprintf(cg->out, " %s", node->var_decl.name);
        if (node->var_decl.init) {
            fputs(" = ", cg->out);
            emit_expr(cg, node->var_decl.init);
        }
    } else {
        emit_expr(cg, node);
    }
}

/* last expr in block becomes a return — used inside expression IIFEs */
static void emit_block_as_return(Codegen *cg, AstNode *block)
{
    int n = block->block.stmts.count;
    for (int i = 0; i < n; i++) {
        AstNode *s = block->block.stmts.items[i];
        if (i == n - 1 && s->kind != NODE_RET) {
            emit_indent(cg);
            fputs("return ", cg->out);
            emit_expr(cg, s);
            fputs(";\n", cg->out);
        } else {
            emit_stmt(cg, s);
        }
    }
}

/* emit if/elif/else as C++ if/else-if/else statement */
static void emit_if_chain(Codegen *cg, AstNode *node, int continuation)
{
    if (!continuation) emit_indent(cg);
    fputs("if (", cg->out);
    emit_expr(cg, node->if_expr.cond);
    fputs(") {\n", cg->out);
    cg->indent++;
    AstNode *then = node->if_expr.then_block;
    for (int i = 0; i < then->block.stmts.count; i++)
        emit_stmt(cg, then->block.stmts.items[i]);
    cg->indent--;
    emit_indent(cg);
    fputc('}', cg->out);

    if (node->if_expr.else_block) {
        AstNode *els = node->if_expr.else_block;
        if (els->kind == NODE_IF) {
            fputs(" else ", cg->out);
            emit_if_chain(cg, els, 1);
        } else {
            fputs(" else {\n", cg->out);
            cg->indent++;
            for (int i = 0; i < els->block.stmts.count; i++)
                emit_stmt(cg, els->block.stmts.items[i]);
            cg->indent--;
            emit_indent(cg);
            fputs("}\n", cg->out);
        }
    } else {
        fputc('\n', cg->out);
    }
}

/* emit if as C++ IIFE expression for use in expression context */
static void emit_if_expr(Codegen *cg, AstNode *node)
{
    fputs("[&]() {\n", cg->out);
    cg->indent++;
    emit_indent(cg);
    fputs("if (", cg->out);
    emit_expr(cg, node->if_expr.cond);
    fputs(") {\n", cg->out);
    cg->indent++;
    emit_block_as_return(cg, node->if_expr.then_block);
    cg->indent--;
    emit_indent(cg);
    fputc('}', cg->out);

    if (node->if_expr.else_block) {
        AstNode *els = node->if_expr.else_block;
        if (els->kind == NODE_IF) {
            fputs(" else if (", cg->out);
            emit_expr(cg, els->if_expr.cond);
            fputs(") {\n", cg->out);
            cg->indent++;
            emit_block_as_return(cg, els->if_expr.then_block);
            cg->indent--;
            emit_indent(cg);
            if (els->if_expr.else_block) {
                fputs("} else {\n", cg->out);
                cg->indent++;
                emit_block_as_return(cg, els->if_expr.else_block);
                cg->indent--;
                emit_indent(cg);
            }
            fputs("}\n", cg->out);
        } else {
            fputs(" else {\n", cg->out);
            cg->indent++;
            emit_block_as_return(cg, els);
            cg->indent--;
            emit_indent(cg);
            fputs("}\n", cg->out);
        }
    } else {
        fputc('\n', cg->out);
    }
    cg->indent--;
    emit_indent(cg);
    fputs("}()", cg->out);
}

/* emit when as a C++ immediately-invoked lambda (expression context) */
static void emit_when_expr(Codegen *cg, AstNode *node)
{
    fputs("[&]() {\n", cg->out);
    cg->indent++;

    AstNode *subj = node->when_expr.subject;
    int type_dispatch = 0;
    const char *type_cpp = NULL;
    static char _twbuf[64];

    if (subj->kind == NODE_IDENT && is_type_var(cg, subj->ident.name)) {
        type_dispatch = 1;
        type_cpp = subj->ident.name;
    } else if (subj->kind == NODE_BUILTIN_CALL &&
               strcmp(subj->call.name, "type") == 0 &&
               subj->call.args.count == 1 &&
               subj->call.args.items[0]->kind == NODE_IDENT) {
        type_dispatch = 1;
        snprintf(_twbuf, sizeof(_twbuf), "T_%s",
                 subj->call.args.items[0]->ident.name);
        type_cpp = _twbuf;
    }

    int first = 1;
    for (int i = 0; i < node->when_expr.arms.count; i++) {
        AstNode *arm = node->when_expr.arms.items[i];
        if (arm->when_arm.pattern == NULL) {
            fputs(" else {\n", cg->out);
        } else {
            if (first) { emit_indent(cg); first = 0; }
            else        fputs(" else ", cg->out);
            if (type_dispatch) {
                const char *pn = arm->when_arm.pattern->kind == NODE_IDENT
                    ? arm->when_arm.pattern->ident.name : "?";
                fprintf(cg->out, "if constexpr (std::is_same_v<%s, %s>) {\n",
                        type_cpp, map_type(pn));
            } else {
                fputs("if (", cg->out);
                emit_expr(cg, subj);
                fputs(" == ", cg->out);
                emit_expr(cg, arm->when_arm.pattern);
                fputs(") {\n", cg->out);
            }
        }
        cg->indent++;
        AstNode *body = arm->when_arm.body;
        if (body->kind == NODE_BLOCK) {
            emit_block_as_return(cg, body);
        } else {
            emit_indent(cg);
            fputs("return ", cg->out);
            emit_expr(cg, body);
            fputs(";\n", cg->out);
        }
        cg->indent--;
        emit_indent(cg);
        fputc('}', cg->out);
    }
    fputc('\n', cg->out);
    cg->indent--;
    emit_indent(cg);
    fputs("}()", cg->out);
}

/* emit when as C++ if/else-if/else chain */
static void emit_when_chain(Codegen *cg, AstNode *node)
{
    AstNode *subj = node->when_expr.subject;

    /* detect type dispatch:
       (a) when T { ... }  — subject is an ident registered as a type var
       (b) when @type(x) { ... }  — subject is @type(x) builtin call        */
    int type_dispatch = 0;
    const char *type_cpp = NULL; /* the C++ type name used in is_same_v<> */

    if (subj->kind == NODE_IDENT && is_type_var(cg, subj->ident.name)) {
        type_dispatch = 1;
        type_cpp = subj->ident.name; /* already a using-alias in scope */
    } else if (subj->kind == NODE_BUILTIN_CALL &&
               strcmp(subj->call.name, "type") == 0 &&
               subj->call.args.count == 1 &&
               subj->call.args.items[0]->kind == NODE_IDENT) {
        /* @type(x) — the template type param is T_x */
        type_dispatch = 1;
        /* build "T_<argname>" on the stack */
        static char _tbuf[64];
        snprintf(_tbuf, sizeof(_tbuf), "T_%s",
                 subj->call.args.items[0]->ident.name);
        type_cpp = _tbuf;
    }

    int first = 1;
    for (int i = 0; i < node->when_expr.arms.count; i++) {
        AstNode *arm = node->when_expr.arms.items[i];
        if (arm->when_arm.pattern == NULL) {
            /* default arm */
            fputs(" else {\n", cg->out);
        } else {
            if (first) { emit_indent(cg); first = 0; }
            else        fputs(" else ", cg->out);

            if (type_dispatch) {
                /* when T { TypeName => ... } → if constexpr (is_same_v<T, CppType>) */
                const char *pat_name = arm->when_arm.pattern->kind == NODE_IDENT
                    ? arm->when_arm.pattern->ident.name : "?";
                fprintf(cg->out, "if constexpr (std::is_same_v<%s, %s>) {\n",
                        type_cpp, map_type(pat_name));
            } else {
                /* when value { val => ... } → if (subject == val) */
                fputs("if (", cg->out);
                emit_expr(cg, node->when_expr.subject);
                fputs(" == ", cg->out);
                emit_expr(cg, arm->when_arm.pattern);
                fputs(") {\n", cg->out);
            }
        }
        cg->indent++;
        AstNode *body = arm->when_arm.body;
        if (body->kind == NODE_BLOCK) {
            for (int j = 0; j < body->block.stmts.count; j++)
                emit_stmt(cg, body->block.stmts.items[j]);
        } else {
            emit_stmt(cg, body);
        }
        cg->indent--;
        emit_indent(cg);
        fputc('}', cg->out);
    }
    fputc('\n', cg->out);
}

static void emit_stmt(Codegen *cg, AstNode *node)
{
    if (!node) return;
    switch (node->kind) {
        case NODE_BUILTIN_CALL:
            emit_builtin_stmt(cg, node);
            break;
        case NODE_RET: {
            int is_err_val = cg->in_err_fn && is_err_return(cg, node->ret.value);
            /* set success flag before successful returns in errdefer functions */
            if (cg->in_err_fn && cg->has_errdefer && !is_err_val) {
                emit_indent(cg);
                fputs("_fn_ok = true;\n", cg->out);
            }
            emit_indent(cg);
            if (cg->in_err_fn) {
                if (!node->ret.value) {
                    fprintf(cg->out, "return _OlrnResult<%s>();\n", cg->err_ret_cpp);
                } else if (is_err_val) {
                    fprintf(cg->out, "return _OlrnResult<%s>(", cg->err_ret_cpp);
                    emit_err_value(cg, node->ret.value);
                    fputs(");\n", cg->out);
                } else {
                    fprintf(cg->out, "return _OlrnResult<%s>(", cg->err_ret_cpp);
                    emit_expr(cg, node->ret.value);
                    fputs(");\n", cg->out);
                }
            } else {
                if (node->ret.value) {
                    fputs("return ", cg->out);
                    emit_expr(cg, node->ret.value);
                    fputs(";\n", cg->out);
                } else {
                    fputs("return;\n", cg->out);
                }
            }
            break;
        }
        case NODE_VAR_DECL: {
            /* T :: @type(x) inside a template → emit as C++ using alias */
            AstNode *init = node->var_decl.init;
            if (cg->in_template && node->var_decl.is_imu &&
                init && init->kind == NODE_BUILTIN_CALL &&
                strcmp(init->call.name, "type") == 0 &&
                init->call.args.count == 1 &&
                init->call.args.items[0]->kind == NODE_IDENT)
            {
                const char *src = init->call.args.items[0]->ident.name;
                emit_indent(cg);
                fprintf(cg->out, "using %s = T_%s;\n", node->var_decl.name, src);
                /* register as a type variable for when T dispatch */
                if (cg->type_var_count < MAX_TYPE_VARS)
                    cg->type_vars[cg->type_var_count++] = node->var_decl.name;
                break;
            }
            /* normal variable declaration */
            emit_indent(cg);
            if (node->var_decl.is_imu) fputs("const ", cg->out);
            if (node->var_decl.type_ref) emit_type(cg, node->var_decl.type_ref);
            else if (init && init->kind == NODE_STR_LIT)
                /* auto would give const char*, force std::string */
                fputs("std::string", cg->out);
            else if (init && init->kind == NODE_ARRAY_LIT
                     && init->array_lit.elems.count > 0) {
                /* auto would give initializer_list — infer vector from first elem */
                AstNode *first = init->array_lit.elems.items[0];
                if      (first->kind == NODE_INT_LIT)   fputs("std::vector<int64_t>",     cg->out);
                else if (first->kind == NODE_FLOAT_LIT) fputs("std::vector<double>",      cg->out);
                else if (first->kind == NODE_STR_LIT)   fputs("std::vector<std::string>", cg->out);
                else if (first->kind == NODE_BOOL_LIT)  fputs("std::vector<bool>",        cg->out);
                else                                    fputs("auto",                      cg->out);
            }
            else fputs("auto", cg->out);
            fprintf(cg->out, " %s", node->var_decl.name);
            if (node->var_decl.init) {
                fputs(" = ", cg->out);
                /* p :^T = @alo(T) → make_shared (a malloc'd pointer can't
                   seed a shared_ptr, and its deleter calls delete) */
                if (node->var_decl.type_ref &&
                    node->var_decl.type_ref->type_ref.is_smart &&
                    init->kind == NODE_BUILTIN_CALL &&
                    strcmp(init->call.name, "alo") == 0 &&
                    init->call.args.count == 1 &&
                    init->call.args.items[0]->kind == NODE_IDENT)
                    fprintf(cg->out, "std::make_shared<%s>()",
                            map_type(init->call.args.items[0]->ident.name));
                else
                    emit_expr(cg, init);
            }
            fputs(";\n", cg->out);
            break;
        }
        case NODE_MULTI_BIND: {
            /* a, b := expr → auto [a, b] = expr; ('_' gets a unique name —
               structured bindings need one, harmless if unused) */
            emit_indent(cg);
            if (node->multi_bind.is_imu) fputs("const ", cg->out);
            fputs("auto [", cg->out);
            for (int i = 0; i < node->multi_bind.names.count; i++) {
                if (i > 0) fputs(", ", cg->out);
                const char *nm = node->multi_bind.names.items[i]->ident.name;
                if (strcmp(nm, "_") == 0)
                    fprintf(cg->out, "_olrn_skip_%d_%d", cg->try_counter++, i);
                else
                    fputs(nm, cg->out);
            }
            fputs("] = ", cg->out);
            emit_expr(cg, node->multi_bind.init);
            fputs(";\n", cg->out);
            break;
        }
        case NODE_VAR_DECL_GROUP: {
            emit_indent(cg);
            if (node->var_decl_group.is_imu) fputs("const ", cg->out);
            emit_type(cg, node->var_decl_group.type_ref);
            for (int i = 0; i < node->var_decl_group.entries.count; i++) {
                AstNode *e = node->var_decl_group.entries.items[i];
                fprintf(cg->out, "%s%s", i == 0 ? " " : ", ", e->var_decl.name);
                if (e->var_decl.init) {
                    fputs(" = ", cg->out);
                    emit_expr(cg, e->var_decl.init);
                }
            }
            fputs(";\n", cg->out);
            break;
        }
        case NODE_IF:
            emit_if_chain(cg, node, 0);
            break;
        case NODE_WHEN:
            emit_when_chain(cg, node);
            break;
        case NODE_CALL:
        case NODE_CALL_EXPR:
            emit_indent(cg);
            emit_expr(cg, node);
            fputs(";\n", cg->out);
            break;
        case NODE_WHILE: {
            emit_indent(cg);
            fputs("while (", cg->out);
            emit_expr(cg, node->while_loop.cond);
            fputs(") {\n", cg->out);
            cg->indent++;
            AstNode *wb = node->while_loop.body;
            for (int i = 0; i < wb->block.stmts.count; i++)
                emit_stmt(cg, wb->block.stmts.items[i]);
            cg->indent--;
            emit_indent(cg);
            fputs("}\n", cg->out);
            break;
        }
        case NODE_LOOP: {
            emit_indent(cg);
            fputs("for (", cg->out);
            emit_for_init(cg, node->loop_stmt.init);
            fputs("; ", cg->out);
            emit_expr(cg, node->loop_stmt.cond);
            fputs("; ", cg->out);
            emit_expr(cg, node->loop_stmt.step);
            fputs(") {\n", cg->out);
            cg->indent++;
            AstNode *lb = node->loop_stmt.body;
            for (int i = 0; i < lb->block.stmts.count; i++)
                emit_stmt(cg, lb->block.stmts.items[i]);
            cg->indent--;
            emit_indent(cg);
            fputs("}\n", cg->out);
            break;
        }
        case NODE_FOR_RANGE: {
            emit_indent(cg);
            fputs("for (auto _i = ", cg->out);
            emit_expr(cg, node->for_range.lo);
            fputs("; _i ", cg->out);
            fputs(node->for_range.inclusive ? "<= " : "< ", cg->out);
            emit_expr(cg, node->for_range.hi);
            fputs("; ++_i) {\n", cg->out);
            cg->indent++;
            AstNode *rb = node->for_range.body;
            for (int i = 0; i < rb->block.stmts.count; i++)
                emit_stmt(cg, rb->block.stmts.items[i]);
            cg->indent--;
            emit_indent(cg);
            fputs("}\n", cg->out);
            break;
        }
        case NODE_FOR_EACH: {
            const char *elem = node->for_each.elem;
            const char *idx  = node->for_each.idx;
            if (idx) {
                /* for e, i => iter — wrap block so the index counter is scoped */
                emit_indent(cg);
                fputs("{\n", cg->out);
                cg->indent++;
                emit_indent(cg);
                fprintf(cg->out, "int64_t %s = 0;\n", idx);
                emit_indent(cg);
                fprintf(cg->out, "for (auto& %s : ", elem);
                emit_expr(cg, node->for_each.iter);
                fputs(") {\n", cg->out);
                cg->indent++;
                AstNode *eb = node->for_each.body;
                for (int i = 0; i < eb->block.stmts.count; i++)
                    emit_stmt(cg, eb->block.stmts.items[i]);
                emit_indent(cg);
                fprintf(cg->out, "++%s;\n", idx);
                cg->indent--;
                emit_indent(cg);
                fputs("}\n", cg->out);
                cg->indent--;
                emit_indent(cg);
                fputs("}\n", cg->out);
            } else {
                emit_indent(cg);
                fprintf(cg->out, "for (auto& %s : ", elem);
                emit_expr(cg, node->for_each.iter);
                fputs(") {\n", cg->out);
                cg->indent++;
                AstNode *eb = node->for_each.body;
                for (int i = 0; i < eb->block.stmts.count; i++)
                    emit_stmt(cg, eb->block.stmts.items[i]);
                cg->indent--;
                emit_indent(cg);
                fputs("}\n", cg->out);
            }
            break;
        }
        case NODE_DEFER: {
            /* emit a RAII guard that calls the deferred code at scope exit */
            emit_indent(cg);
            fprintf(cg->out, "_OlrnDeferGuard _defer_%d([&]() {\n", cg->defer_counter);
            cg->indent++;
            AstNode *de = node->defer_stmt.expr;
            if (de->kind == NODE_BLOCK) {
                for (int i = 0; i < de->block.stmts.count; i++)
                    emit_stmt(cg, de->block.stmts.items[i]);
            } else {
                emit_stmt(cg, de);
            }
            cg->indent--;
            emit_indent(cg);
            fputs("});\n", cg->out);
            cg->defer_counter++;
            break;
        }
        case NODE_ERRDEFER: {
            /* RAII guard that only runs on error exit (_fn_ok stays false) */
            emit_indent(cg);
            fprintf(cg->out, "_OlrnDeferGuard _errdefer_%d([&]() {\n", cg->defer_counter);
            cg->indent++;
            emit_indent(cg);
            fputs("if (!_fn_ok) {\n", cg->out);
            cg->indent++;
            AstNode *ed = node->errdefer_stmt.expr;
            if (ed->kind == NODE_BLOCK) {
                for (int i = 0; i < ed->block.stmts.count; i++)
                    emit_stmt(cg, ed->block.stmts.items[i]);
            } else {
                emit_stmt(cg, ed);
            }
            cg->indent--;
            emit_indent(cg);
            fputs("}\n", cg->out);
            cg->indent--;
            emit_indent(cg);
            fputs("});\n", cg->out);
            cg->defer_counter++;
            break;
        }
        case NODE_TRY_EXPR: {
            /* try as a statement — propagate error, discard success value */
            int n = cg->try_counter++;
            emit_indent(cg);
            fprintf(cg->out, "{ auto _r_%d = (", n);
            emit_expr(cg, node->try_expr.expr);
            fprintf(cg->out,
                    "); if (!_r_%d.is_ok()) return _OlrnResult<%s>(_r_%d.error()); }\n",
                    n, cg->err_ret_cpp, n);
            break;
        }
        case NODE_CATCH_EXPR: {
            /* catch as a statement */
            int n = cg->try_counter++;
            emit_indent(cg);
            fputs("{\n", cg->out);
            cg->indent++;
            emit_indent(cg);
            fprintf(cg->out, "auto _r_%d = (", n);
            emit_expr(cg, node->catch_expr.expr);
            fputs(");\n", cg->out);
            if (node->catch_expr.body) {
                emit_indent(cg);
                fprintf(cg->out, "if (!_r_%d.is_ok()) {\n", n);
                cg->indent++;
                if (node->catch_expr.err_var) {
                    emit_indent(cg);
                    fprintf(cg->out, "_OlrnError %s = _r_%d.error();\n",
                            node->catch_expr.err_var, n);
                }
                AstNode *bdy = node->catch_expr.body;
                if (bdy->kind == NODE_BLOCK)
                    for (int j = 0; j < bdy->block.stmts.count; j++)
                        emit_stmt(cg, bdy->block.stmts.items[j]);
                cg->indent--;
                emit_indent(cg);
                fputs("}\n", cg->out);
            }
            cg->indent--;
            emit_indent(cg);
            fputs("}\n", cg->out);
            break;
        }
        default:
            emit_indent(cg);
            emit_expr(cg, node);
            fputs(";\n", cg->out);
            break;
    }
}

static int param_is_any(AstNode *param)
{
    return param->param.type &&
           param->param.type->kind == NODE_TYPE_REF &&
           strcmp(param->param.type->type_ref.name, "any") == 0;
}

/* emit function forward declaration (signature + ;) so call order is unrestricted */
static void emit_fn_fwd(Codegen *cg, AstNode *fn)
{
    if (strcmp(fn->fn_decl.name, "main") == 0) return; /* main never needs fwd decl */

    int has_any = 0;
    for (int i = 0; i < fn->fn_decl.params.count; i++)
        if (param_is_any(fn->fn_decl.params.items[i])) { has_any = 1; break; }

    if (has_any) {
        fputs("template<", cg->out);
        int first = 1;
        for (int i = 0; i < fn->fn_decl.params.count; i++) {
            AstNode *param = fn->fn_decl.params.items[i];
            if (param_is_any(param)) {
                if (!first) fputs(", ", cg->out);
                fprintf(cg->out, "typename T_%s", param->param.name);
                first = 0;
            }
        }
        fputs(">\n", cg->out);
    }

    emit_type(cg, fn->fn_decl.ret_type);
    fprintf(cg->out, " %s(", fn->fn_decl.name);
    for (int i = 0; i < fn->fn_decl.params.count; i++) {
        AstNode *param = fn->fn_decl.params.items[i];
        if (i > 0) fputs(", ", cg->out);
        if (param_is_any(param))
            fprintf(cg->out, "T_%s", param->param.name);
        else
            emit_type(cg, param->param.type);
        fprintf(cg->out, " %s", param->param.name);
    }
    fputs(");\n", cg->out);
}

static void emit_fn(Codegen *cg, AstNode *fn)
{
    codegen_reset_fn_state(cg);
    int is_main = strcmp(fn->fn_decl.name, "main") == 0;
    AstNode *ret_type = fn->fn_decl.ret_type;
    int is_result_fn = ret_type && ret_type->type_ref.is_result;

    /* populate err_ret_cpp for any error-returning function */
    if (is_result_fn) {
        cg->in_err_fn = 1;
        const char *base = map_type(ret_type->type_ref.name);
        if (ret_type->type_ref.is_arr && ret_type->type_ref.arr_size > 0)
            snprintf(cg->err_ret_cpp, sizeof(cg->err_ret_cpp),
                     "std::array<%s, %d>", base, ret_type->type_ref.arr_size);
        else if (ret_type->type_ref.is_arr)
            snprintf(cg->err_ret_cpp, sizeof(cg->err_ret_cpp),
                     "std::vector<%s>", base);
        else
            snprintf(cg->err_ret_cpp, sizeof(cg->err_ret_cpp), "%s", base);
    }

    /* fn main() -> !T : emit as static _olrn_main() + int main() wrapper */
    if (is_main && is_result_fn) {
        cg->has_errdefer = fn_has_errdefer(fn->fn_decl.body);
        fputs("static ", cg->out);
        emit_type(cg, ret_type);
        fputs(" _olrn_main()\n{\n", cg->out);
        cg->indent++;
        if (cg->has_errdefer) { emit_indent(cg); fputs("bool _fn_ok = false;\n", cg->out); }
        AstNode *body = fn->fn_decl.body;
        for (int i = 0; i < body->block.stmts.count; i++)
            emit_stmt(cg, body->block.stmts.items[i]);
        /* implicit success return for !void if last stmt is not a ret */
        if (strcmp(cg->err_ret_cpp, "void") == 0) {
            AstNode *last = body->block.stmts.count > 0
                ? body->block.stmts.items[body->block.stmts.count - 1] : NULL;
            if (!last || last->kind != NODE_RET) {
                if (cg->has_errdefer) { emit_indent(cg); fputs("_fn_ok = true;\n", cg->out); }
                emit_indent(cg);
                fputs("return _OlrnResult<void>();\n", cg->out);
            }
        }
        cg->indent--;
        fputs("}\n\n", cg->out);
        fputs("int main()\n{\n", cg->out);
        fputs("    auto _r = _olrn_main();\n", cg->out);
        fputs("    if (!_r.is_ok()) {\n", cg->out);
        fputs("        std::cerr << \"error: \" << _r.error().msg << std::endl;\n", cg->out);
        fputs("        return 1;\n", cg->out);
        fputs("    }\n", cg->out);
        fputs("    return 0;\n", cg->out);
        fputs("}\n", cg->out);
        return;
    }

    /* detect any params → emit template header */
    int has_any = 0;
    for (int i = 0; i < fn->fn_decl.params.count; i++)
        if (param_is_any(fn->fn_decl.params.items[i])) { has_any = 1; break; }

    if (has_any) {
        fputs("template<", cg->out);
        int first = 1;
        for (int i = 0; i < fn->fn_decl.params.count; i++) {
            AstNode *param = fn->fn_decl.params.items[i];
            if (param_is_any(param)) {
                if (!first) fputs(", ", cg->out);
                fprintf(cg->out, "typename T_%s", param->param.name);
                first = 0;
            }
        }
        fputs(">\n", cg->out);
        cg->in_template = 1;
    }

    /* C++ main always returns int; error-returning fns get _OlrnResult<T> */
    if (is_main) fputs("int", cg->out);
    else         emit_type(cg, fn->fn_decl.ret_type);

    fprintf(cg->out, " %s(", fn->fn_decl.name);

    for (int i = 0; i < fn->fn_decl.params.count; i++) {
        AstNode *param = fn->fn_decl.params.items[i];
        if (i > 0) fputs(", ", cg->out);
        if (param_is_any(param))
            fprintf(cg->out, "T_%s", param->param.name);
        else
            emit_type(cg, param->param.type);
        fprintf(cg->out, " %s", param->param.name);
    }

    fputs(")\n{\n", cg->out);
    cg->indent++;

    /* emit _fn_ok flag if function has errdefer */
    if (cg->in_err_fn && fn_has_errdefer(fn->fn_decl.body)) {
        cg->has_errdefer = 1;
        emit_indent(cg);
        fputs("bool _fn_ok = false;\n", cg->out);
    }

    AstNode *body = fn->fn_decl.body;
    for (int i = 0; i < body->block.stmts.count; i++)
        emit_stmt(cg, body->block.stmts.items[i]);

    /* main: explicit return 0 if not present */
    if (is_main) {
        AstNode *last = body->block.stmts.count > 0
            ? body->block.stmts.items[body->block.stmts.count - 1] : NULL;
        if (!last || last->kind != NODE_RET) {
            emit_indent(cg);
            fputs("return 0;\n", cg->out);
        }
    }

    /* !void non-main: implicit success return if not present */
    if (cg->in_err_fn && strcmp(cg->err_ret_cpp, "void") == 0 && !is_main) {
        AstNode *last = body->block.stmts.count > 0
            ? body->block.stmts.items[body->block.stmts.count - 1] : NULL;
        if (!last || last->kind != NODE_RET) {
            if (cg->has_errdefer) { emit_indent(cg); fputs("_fn_ok = true;\n", cg->out); }
            emit_indent(cg);
            fputs("return _OlrnResult<void>();\n", cg->out);
        }
    }

    cg->indent--;
    fputs("}\n", cg->out);
}

void codegen_emit(Codegen *cg, AstNode *program)
{
    /* standard headers every Oleren program needs */
    fputs("#include <iostream>\n", cg->out);
    fputs("#include <string>\n", cg->out);
    fputs("#include <vector>\n", cg->out);
    fputs("#include <array>\n", cg->out);
    fputs("#include <memory>\n", cg->out);
    fputs("#include <tuple>\n", cg->out);
    fputs("#include <type_traits>\n", cg->out);
    fputs("#include <algorithm>\n", cg->out);
    fputs("#include <cmath>\n", cg->out);
    fputs("#include <cstring>\n", cg->out);
    fputs("#include <random>\n", cg->out);
    fputs("#include <cstdint>\n", cg->out);
    fputs("#include <cstdlib>\n", cg->out);
    fputs("#include <cstdio>\n", cg->out);
    fputs("#include <typeinfo>\n", cg->out);
    fputs("#include <cxxabi.h>\n", cg->out);
    /* RAII guard used by 'defer' — always emitted so defer is zero-cost when unused */
    fputs("template<typename F>\n", cg->out);
    fputs("struct _OlrnDeferGuard {\n", cg->out);
    fputs("    F fn;\n", cg->out);
    fputs("    _OlrnDeferGuard(F f) : fn(std::move(f)) {}\n", cg->out);
    fputs("    ~_OlrnDeferGuard() { fn(); }\n", cg->out);
    fputs("    _OlrnDeferGuard(const _OlrnDeferGuard&) = delete;\n", cg->out);
    fputs("    _OlrnDeferGuard& operator=(const _OlrnDeferGuard&) = delete;\n", cg->out);
    fputs("};\n", cg->out);
    fputs(BUILTINS_IMPL, cg->out);

    /* register import aliases; detect stdlib */
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (cg->import_alias_count < MAX_IMPORT_ALIAS) {
            cg->import_aliases[cg->import_alias_count] = imp->import_decl.alias;
            cg->import_modules[cg->import_alias_count] = imp->import_decl.module;
            cg->import_alias_count++;
        }
        if (imp->import_decl.is_lib && strcmp(imp->import_decl.source, "std") == 0) {
            cg->has_stdlib = 1;
            if (imp->import_decl.module &&
                strcmp(imp->import_decl.module, "malkur") == 0)
                cg->has_malkur = 1;
            if (imp->import_decl.module &&
                strcmp(imp->import_decl.module, "pelentar") == 0)
                cg->has_pelentar = 1;
        }
    }

    /* emit stdlib C++ preamble if needed; malkur adds the SDL2 backend
       (defines Color/Vec2/Rect used by the malkur .olrn functions) */
    if (cg->has_stdlib)
        fputs(STDLIB_IMPL, cg->out);
    if (cg->has_malkur)
        fputs(MALKUR_IMPL, cg->out);
    if (cg->has_pelentar)
        fputs(PELENTAR_IMPL, cg->out);
    if (!cg->has_stdlib && program->program.imports.count)
        fputc('\n', cg->out);

    /* error set declarations */
    int has_err_sets = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_ERR_DECL) continue;
        has_err_sets = 1;
        /* register name so codegen knows ErrSet.X is an error, not a struct field */
        if (cg->err_count < MAX_ERR_NAMES)
            cg->err_names[cg->err_count++] = decl->err_decl.name;
        fprintf(cg->out, "namespace _olrn_err_%s {\n", decl->err_decl.name);
        for (int j = 0; j < decl->err_decl.variants.count; j++) {
            AstNode *v = decl->err_decl.variants.items[j];
            fprintf(cg->out, "    static constexpr _OlrnError %s{%d, \"%s\"};\n",
                    v->ident.name, j, v->ident.name);
        }
        fputs("}\n", cg->out);
    }
    if (has_err_sets) fputc('\n', cg->out);

    /* type aliases — emitted before structs/enums so types are in scope */
    int has_types = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_TYPE_ALIAS) continue;
        has_types = 1;
        fputs("using ", cg->out);
        fputs(decl->type_alias.name, cg->out);
        fputs(" = ", cg->out);
        emit_type(cg, decl->type_alias.target);
        fputs(";\n", cg->out);
    }
    if (has_types) fputc('\n', cg->out);

    /* enum declarations */
    int has_enums = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_ENUM_DECL) continue;
        has_enums = 1;
        /* register name for :: access in emit_expr */
        if (cg->enum_count < MAX_ENUM_NAMES)
            cg->enum_names[cg->enum_count++] = decl->enum_decl.name;
        if (!decl->enum_decl.base_type) {
            /* plain enum → enum class */
            fprintf(cg->out, "enum class %s {", decl->enum_decl.name);
            for (int j = 0; j < decl->enum_decl.variants.count; j++) {
                AstNode *v = decl->enum_decl.variants.items[j];
                fprintf(cg->out, "%s%s", j == 0 ? " " : ", ", v->enum_variant.name);
            }
            fputs(" };\n", cg->out);
        } else {
            /* typed enum → namespace of constexpr values */
            fprintf(cg->out, "namespace %s {\n", decl->enum_decl.name);
            for (int j = 0; j < decl->enum_decl.variants.count; j++) {
                AstNode *v = decl->enum_decl.variants.items[j];
                fputs("    static constexpr ", cg->out);
                emit_type(cg, decl->enum_decl.base_type);
                fprintf(cg->out, " %s", v->enum_variant.name);
                if (v->enum_variant.value) {
                    fputs(" = ", cg->out);
                    /* emit raw literal to avoid narrowing in constexpr context */
                    AstNode *val = v->enum_variant.value;
                    if (val->kind == NODE_INT_LIT)
                        fprintf(cg->out, "%lld", val->int_lit.value);
                    else if (val->kind == NODE_FLOAT_LIT) {
                        char _fb[32];
                        snprintf(_fb, sizeof(_fb), "%g", val->float_lit.value);
                        fputs(_fb, cg->out);
                        if (!strchr(_fb,'.') && !strchr(_fb,'e') && !strchr(_fb,'E'))
                            fputs(".0", cg->out);
                    }
                    else
                        emit_expr(cg, val);
                }
                fputs(";\n", cg->out);
            }
            fputs("}\n", cg->out);
        }
    }
    if (has_enums) fputc('\n', cg->out);

    /* struct declarations */
    int has_structs = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_STRUCT_DECL) continue;
        has_structs = 1;
        fprintf(cg->out, "struct %s {\n", decl->struct_decl.name);
        for (int j = 0; j < decl->struct_decl.fields.count; j++) {
            AstNode *f = decl->struct_decl.fields.items[j];
            fputs("    ", cg->out);
            emit_type(cg, f->param.type);
            fprintf(cg->out, " %s;\n", f->param.name);
        }
        fputs("};\n", cg->out);
    }
    if (has_structs) fputc('\n', cg->out);

    /* union declarations */
    int has_unions = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_UNN_DECL) continue;
        has_unions = 1;
        fprintf(cg->out, "union %s {\n", decl->unn_decl.name);
        for (int j = 0; j < decl->unn_decl.fields.count; j++) {
            AstNode *f = decl->unn_decl.fields.items[j];
            fputs("    ", cg->out);
            emit_type(cg, f->param.type);
            fprintf(cg->out, " %s;\n", f->param.name);
        }
        fputs("};\n", cg->out);
    }
    if (has_unions) fputc('\n', cg->out);

    /* extern fn declarations — emitted first so they're visible to all fns */
    int has_extern = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_EXTERN_FN) continue;
        has_extern = 1;
        fputs("extern \"C\" ", cg->out);
        if (decl->extern_fn.ret_type) emit_type(cg, decl->extern_fn.ret_type);
        else                          fputs("void", cg->out);
        fprintf(cg->out, " %s(", decl->extern_fn.name);
        for (int j = 0; j < decl->extern_fn.params.count; j++) {
            AstNode *param = decl->extern_fn.params.items[j];
            if (j > 0) fputs(", ", cg->out);
            emit_type(cg, param->param.type);
            fprintf(cg->out, " %s", param->param.name);
        }
        if (decl->extern_fn.is_variadic) {
            if (decl->extern_fn.params.count > 0) fputs(", ", cg->out);
            fputs("...", cg->out);
        }
        fputs(");\n", cg->out);
    }
    if (has_extern) fputc('\n', cg->out);

    /* top-level constants and variables */
    int has_globals = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_VAR_DECL && decl->kind != NODE_VAR_DECL_GROUP) continue;
        has_globals = 1;
        if (decl->kind == NODE_VAR_DECL) {
            if (decl->var_decl.is_imu) fputs("constexpr ", cg->out);
            else                       fputs("static ", cg->out);
            if (decl->var_decl.type_ref) emit_type(cg, decl->var_decl.type_ref);
            else                         fputs("auto", cg->out);
            fprintf(cg->out, " %s", decl->var_decl.name);
            if (decl->var_decl.init) {
                fputs(" = ", cg->out);
                emit_expr(cg, decl->var_decl.init);
            }
            fputs(";\n", cg->out);
        }
    }
    if (has_globals) fputc('\n', cg->out);

    /* function forward declarations — allows any call order */
    int has_fwd = 0;
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind != NODE_FN_DECL) continue;
        emit_fn_fwd(cg, decl);
        has_fwd = 1;
    }
    if (has_fwd) fputc('\n', cg->out);

    /* regular function definitions */
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind == NODE_EXTERN_FN)    continue;
        if (decl->kind == NODE_VAR_DECL || decl->kind == NODE_VAR_DECL_GROUP) continue;
        if (decl->kind == NODE_TYPE_ALIAS)   continue;
        if (decl->kind == NODE_ENUM_DECL)    continue;
        if (decl->kind == NODE_STRUCT_DECL)  continue;
        if (decl->kind == NODE_UNN_DECL)     continue;
        if (decl->kind == NODE_ERR_DECL)     continue;
        emit_fn(cg, decl);
        fputc('\n', cg->out);
    }
}
