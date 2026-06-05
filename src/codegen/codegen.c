#include "codegen.h"
#include "../lexer/lexer.h"
#include <string.h>
#include <stdio.h>

void codegen_init(Codegen *cg, FILE *out)
{
    cg->out    = out;
    cg->indent = 0;
}

static void emit_indent(Codegen *cg)
{
    for (int i = 0; i < cg->indent * 4; i++) fputc(' ', cg->out);
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
    if (type_ref->type_ref.is_ptr)   fputs("", cg->out);   /* raw ptr handled by * after type */
    if (type_ref->type_ref.is_smart) fputs("std::shared_ptr<", cg->out);

    fputs(map_type(type_ref->type_ref.name), cg->out);

    if (type_ref->type_ref.is_ptr)   fputc('*', cg->out);
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
            fprintf(cg->out, "%lld", node->int_lit.value);
            break;
        case NODE_FLOAT_LIT:
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
        case NODE_BUILTIN_CALL:
            /* value builtins (@cout, @endl) have no arg list */
            if (node->call.args.count == 0) {
                if      (strcmp(node->call.name, "cout") == 0) fputs("std::cout", cg->out);
                else if (strcmp(node->call.name, "endl") == 0) fputs("std::endl", cg->out);
                else    fprintf(cg->out, "/* @%s */", node->call.name);
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
            emit_expr(cg, node->field.target);
            fprintf(cg->out, ".%s", node->field.name);
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
    int first = 1;
    for (int i = 0; i < node->when_expr.arms.count; i++) {
        AstNode *arm = node->when_expr.arms.items[i];
        if (arm->when_arm.pattern == NULL) {
            fputs(" else {\n", cg->out);
        } else {
            if (first) { emit_indent(cg); first = 0; }
            else        fputs(" else ", cg->out);
            fputs("if (", cg->out);
            emit_expr(cg, node->when_expr.subject);
            fputs(" == ", cg->out);
            emit_expr(cg, arm->when_arm.pattern);
            fputs(") {\n", cg->out);
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
        case NODE_IF:
            emit_if_chain(cg, node, 0);
            break;
        case NODE_WHEN:
            emit_when_chain(cg, node);
            break;
        case NODE_CALL:
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

static void emit_fn(Codegen *cg, AstNode *fn)
{
    int is_main = strcmp(fn->fn_decl.name, "main") == 0;

    /* C++ main always returns int */
    if (is_main) fputs("int", cg->out);
    else         emit_type(cg, fn->fn_decl.ret_type);

    fprintf(cg->out, " %s(", fn->fn_decl.name);

    for (int i = 0; i < fn->fn_decl.params.count; i++) {
        AstNode *param = fn->fn_decl.params.items[i];
        if (i > 0) fputs(", ", cg->out);
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
    fputs("#include <cstdint>\n", cg->out);
    fputs("#include <cstdlib>\n", cg->out);
    fputs("#include <cstdio>\n", cg->out);
    fputc('\n', cg->out);

    for (int i = 0; i < program->program.decls.count; i++) {
        emit_fn(cg, program->program.decls.items[i]);
        fputc('\n', cg->out);
    }
}
