#include "codegen.h"
#include "stdlib_impl.h"
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
}

static void codegen_reset_fn_state(Codegen *cg)
{
    cg->in_template    = 0;
    cg->type_var_count = 0;
}

static int is_import_alias(Codegen *cg, const char *name)
{
    for (int i = 0; i < cg->import_alias_count; i++)
        if (strcmp(cg->import_aliases[i], name) == 0) return 1;
    return 0;
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

/* true if the root of a field chain is a known import alias */
static int callee_is_import_qualified(Codegen *cg, AstNode *callee)
{
    while (callee->kind == NODE_FIELD || callee->kind == NODE_FIELD_PTR)
        callee = callee->field.target;
    return callee->kind == NODE_IDENT && is_import_alias(cg, callee->ident.name);
}

static int is_type_var(Codegen *cg, const char *name)
{
    for (int i = 0; i < cg->type_var_count; i++)
        if (strcmp(cg->type_vars[i], name) == 0) return 1;
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
        "f32","f64","chr","str","istr","bool",
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
static void emit_if_chain(Codegen *cg, AstNode *node, int continuation);
static void emit_if_expr(Codegen *cg, AstNode *node);
static void emit_when_chain(Codegen *cg, AstNode *node);

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
    if (strcmp(name, "chr")  == 0) return "char";
    if (strcmp(name, "str")  == 0) return "std::string";
    if (strcmp(name, "bool") == 0) return "bool";
    if (strcmp(name, "void") == 0) return "void";
    return name; /* pass through unknown types */
}

static void emit_type(Codegen *cg, AstNode *type_ref)
{
    if (!type_ref) { fputs("void", cg->out); return; }
    if (type_ref->type_ref.is_arr)   fputs("std::vector<", cg->out);
    if (type_ref->type_ref.is_imu)   fputs("const ", cg->out);
    if (type_ref->type_ref.is_smart) fputs("std::shared_ptr<", cg->out);

    fputs(map_type(type_ref->type_ref.name), cg->out);

    if (type_ref->type_ref.is_ptr)   fputs(" *", cg->out);
    if (type_ref->type_ref.is_smart) fputc('>', cg->out);
    if (type_ref->type_ref.is_arr)   fputc('>', cg->out);
}

/* emit a single builtin call as a statement */
static void emit_builtin_stmt(Codegen *cg, AstNode *node)
{
    const char *name = node->call.name;
    emit_indent(cg);

    /* @pl(val) => std::cout << val << std::endl; */
    if (strcmp(name, "pl") == 0) {
        fputs("std::cout", cg->out);
        for (int i = 0; i < node->call.args.count; i++) {
            fputs(" << ", cg->out);
            emit_expr(cg, node->call.args.items[i]);
        }
        fputs(" << std::endl;\n", cg->out);
        return;
    }

    /* @pf(fmt, ...) => printf(fmt, ...); */
    if (strcmp(name, "pf") == 0) {
        fputs("printf(", cg->out);
        for (int i = 0; i < node->call.args.count; i++) {
            if (i > 0) fputs(", ", cg->out);
            emit_expr(cg, node->call.args.items[i]);
        }
        fputs(");\n", cg->out);
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

    /* fallback: emit as a plain call */
    fprintf(cg->out, "/* @%s */ ", name);
    fputs("(", cg->out);
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
        case NODE_FLOAT_LIT:
            /* Oleren: untyped float literals always infer to f64 (double) */
            fprintf(cg->out, "%g", node->float_lit.value);
            break;
        case NODE_CHAR_LIT:
            fprintf(cg->out, "'%c'", node->char_lit.value);
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
        case NODE_CALL_EXPR:
            /* strip import alias prefix: utils.square(x) → square(x) */
            if (callee_is_import_qualified(cg, node->call_expr.callee)) {
                const char *fn = last_field_name(node->call_expr.callee);
                if (fn) fputs(fn, cg->out);
            } else {
                emit_expr(cg, node->call_expr.callee);
            }
            fputc('(', cg->out);
            for (int i = 0; i < node->call_expr.args.count; i++) {
                if (i > 0) fputs(", ", cg->out);
                emit_expr(cg, node->call_expr.args.items[i]);
            }
            fputc(')', cg->out);
            break;
        case NODE_BUILTIN_CALL:
            if (node->call.args.count == 0) {
                /* value builtins: @cout @endl */
                if      (strcmp(node->call.name, "cout") == 0) fputs("std::cout", cg->out);
                else if (strcmp(node->call.name, "endl") == 0) fputs("std::endl", cg->out);
                else    fprintf(cg->out, "/* @%s */", node->call.name);
            } else if (is_cast_builtin(node->call.name) && node->call.args.count == 1) {
                /* @T(val) — type cast */
                AstNode *arg = node->call.args.items[0];
                if (strcmp(node->call.name, "str") == 0 ||
                    strcmp(node->call.name, "istr") == 0) {
                    /* numeric/bool -> string */
                    fputs("std::to_string(", cg->out);
                    emit_expr(cg, arg);
                    fputc(')', cg->out);
                } else if (strcmp(node->call.name, "bool") == 0) {
                    fputs("(bool)(", cg->out);
                    emit_expr(cg, arg);
                    fputc(')', cg->out);
                } else {
                    /* numeric-to-numeric */
                    fprintf(cg->out, "static_cast<%s>(", map_type(node->call.name));
                    emit_expr(cg, arg);
                    fputc(')', cg->out);
                }
            } else {
                emit_builtin_stmt(cg, node);
            }
            break;
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
                case TOK_MINUS: fputc('-', cg->out); break;
                case TOK_BANG:  fputc('!', cg->out); break;
                case TOK_AMP:   fputc('&', cg->out); break;
                default: break;
            }
            emit_expr(cg, node->unary.operand);
            break;
        case NODE_FIELD:
            /* strip import alias prefix: std.math.PI → PI */
            if (callee_is_import_qualified(cg, node))
                fputs(last_field_name(node), cg->out);
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
        case NODE_IF:
            emit_if_expr(cg, node);
            break;
        default: break;
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

/* emit when as C++ if/else-if/else chain */
static void emit_when_chain(Codegen *cg, AstNode *node)
{
    /* detect type dispatch: subject is an ident registered as a type var */
    int type_dispatch = node->when_expr.subject->kind == NODE_IDENT &&
                        is_type_var(cg, node->when_expr.subject->ident.name);

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
                const char *type_var = node->when_expr.subject->ident.name;
                const char *pat_name = arm->when_arm.pattern->kind == NODE_IDENT
                    ? arm->when_arm.pattern->ident.name : "?";
                fprintf(cg->out, "if constexpr (std::is_same_v<%s, %s>) {\n",
                        type_var, map_type(pat_name));
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
        case NODE_RET:
            emit_indent(cg);
            if (node->ret.value) {
                fputs("return ", cg->out);
                emit_expr(cg, node->ret.value);
                fputs(";\n", cg->out);
            } else {
                fputs("return;\n", cg->out);
            }
            break;
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
            else                         fputs("auto", cg->out);
            fprintf(cg->out, " %s", node->var_decl.name);
            if (node->var_decl.init) {
                fputs(" = ", cg->out);
                emit_expr(cg, node->var_decl.init);
            }
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

static void emit_fn(Codegen *cg, AstNode *fn)
{
    codegen_reset_fn_state(cg);
    int is_main = strcmp(fn->fn_decl.name, "main") == 0;

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

    /* C++ main always returns int */
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

    AstNode *body = fn->fn_decl.body;
    for (int i = 0; i < body->block.stmts.count; i++)
        emit_stmt(cg, body->block.stmts.items[i]);

    /* main needs an explicit return 0 if not already present */
    if (is_main) {
        AstNode *last = body->block.stmts.count > 0
            ? body->block.stmts.items[body->block.stmts.count - 1]
            : NULL;
        int has_ret = last && last->kind == NODE_RET;
        if (!has_ret) {
            emit_indent(cg);
            fputs("return 0;\n", cg->out);
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
    fputs("#include <memory>\n", cg->out);
    fputs("#include <type_traits>\n", cg->out);
    fputs("#include <cstdint>\n", cg->out);
    fputs("#include <cstdlib>\n", cg->out);
    fputs("#include <cstdio>\n", cg->out);

    /* register import aliases; detect stdlib */
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (cg->import_alias_count < MAX_IMPORT_ALIAS)
            cg->import_aliases[cg->import_alias_count++] = imp->import_decl.alias;
        if (imp->import_decl.is_lib && strcmp(imp->import_decl.source, "std") == 0)
            cg->has_stdlib = 1;
    }

    /* emit stdlib C++ preamble if needed */
    if (cg->has_stdlib)
        fputs(STDLIB_IMPL, cg->out);
    else if (program->program.imports.count)
        fputc('\n', cg->out);

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

    /* regular function definitions */
    for (int i = 0; i < program->program.decls.count; i++) {
        AstNode *decl = program->program.decls.items[i];
        if (decl->kind == NODE_EXTERN_FN) continue;
        if (decl->kind == NODE_VAR_DECL || decl->kind == NODE_VAR_DECL_GROUP) continue;
        emit_fn(cg, decl);
        fputc('\n', cg->out);
    }
}
