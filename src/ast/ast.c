#include "ast.h"
#include <stdlib.h>
#include <stdio.h>

AstNode *ast_node_new(NodeKind kind, int line)
{
    AstNode *n = calloc(1, sizeof(AstNode));
    n->kind = kind;
    n->line = line;
    return n;
}

void node_list_push(NodeList *list, AstNode *node)
{
    if (list->count == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 4;
        list->items = realloc(list->items, list->cap * sizeof(AstNode *));
    }
    list->items[list->count++] = node;
}

static void node_list_free(NodeList *list)
{
    for (int i = 0; i < list->count; i++) ast_free(list->items[i]);
    free(list->items);
}

void ast_free(AstNode *node)
{
    if (!node) return;
    switch (node->kind) {
        case NODE_PROGRAM:
            node_list_free(&node->program.decls);
            break;
        case NODE_FN_DECL:
            free(node->fn_decl.name);
            node_list_free(&node->fn_decl.params);
            ast_free(node->fn_decl.ret_type);
            ast_free(node->fn_decl.body);
            break;
        case NODE_PARAM:
            free(node->param.name);
            ast_free(node->param.type);
            break;
        case NODE_BLOCK:
            node_list_free(&node->block.stmts);
            break;
        case NODE_TYPE_REF:
            free(node->type_ref.name);
            break;
        case NODE_BUILTIN_CALL:
        case NODE_CALL:
            free(node->call.name);
            node_list_free(&node->call.args);
            break;
        case NODE_RET:
            ast_free(node->ret.value);
            break;
        case NODE_IF:
            ast_free(node->if_expr.cond);
            ast_free(node->if_expr.then_block);
            ast_free(node->if_expr.else_block);
            break;
        case NODE_WHEN:
            ast_free(node->when_expr.subject);
            node_list_free(&node->when_expr.arms);
            break;
        case NODE_WHEN_ARM:
            ast_free(node->when_arm.pattern);
            ast_free(node->when_arm.body);
            break;
        case NODE_BINARY:
            ast_free(node->binary.left);
            ast_free(node->binary.right);
            break;
        case NODE_UNARY:
            ast_free(node->unary.operand);
            break;
        case NODE_FIELD:
        case NODE_FIELD_PTR:
            ast_free(node->field.target);
            free(node->field.name);
            break;
        case NODE_SUBSCRIPT:
            ast_free(node->subscript.target);
            ast_free(node->subscript.index);
            break;
        case NODE_DEREF:
            ast_free(node->deref.target);
            break;
        case NODE_STR_LIT:   free(node->str_lit.value);  break;
        case NODE_IDENT:     free(node->ident.name);      break;
        default: break;
    }
    free(node);
}

static void do_indent(int n) { for (int i = 0; i < n * 2; i++) putchar(' '); }

void ast_print(AstNode *node, int indent)
{
    if (!node) return;
    do_indent(indent);
    switch (node->kind) {
        case NODE_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->program.decls.count; i++)
                ast_print(node->program.decls.items[i], indent + 1);
            break;
        case NODE_FN_DECL:
            printf("FnDecl(%s)\n", node->fn_decl.name);
            for (int i = 0; i < node->fn_decl.params.count; i++)
                ast_print(node->fn_decl.params.items[i], indent + 1);
            ast_print(node->fn_decl.ret_type, indent + 1);
            ast_print(node->fn_decl.body, indent + 1);
            break;
        case NODE_PARAM:
            printf("Param(%s)\n", node->param.name);
            ast_print(node->param.type, indent + 1);
            break;
        case NODE_BLOCK:
            printf("Block\n");
            for (int i = 0; i < node->block.stmts.count; i++)
                ast_print(node->block.stmts.items[i], indent + 1);
            break;
        case NODE_TYPE_REF:
            printf("TypeRef(%s%s%s%s)\n",
                   node->type_ref.is_arr   ? "[]" : "",
                   node->type_ref.is_imu   ? "imu " : "",
                   node->type_ref.is_ptr   ? "*" : node->type_ref.is_smart ? "^" : "",
                   node->type_ref.name);
            break;
        case NODE_BUILTIN_CALL:
            printf("BuiltinCall(@%s)\n", node->call.name);
            for (int i = 0; i < node->call.args.count; i++)
                ast_print(node->call.args.items[i], indent + 1);
            break;
        case NODE_CALL:
            printf("Call(%s)\n", node->call.name);
            for (int i = 0; i < node->call.args.count; i++)
                ast_print(node->call.args.items[i], indent + 1);
            break;
        case NODE_RET:
            printf("Ret\n");
            ast_print(node->ret.value, indent + 1);
            break;
        case NODE_IF:
            printf("If\n");
            ast_print(node->if_expr.cond,       indent + 1);
            ast_print(node->if_expr.then_block,  indent + 1);
            ast_print(node->if_expr.else_block,  indent + 1);
            break;
        case NODE_WHEN:
            printf("When\n");
            ast_print(node->when_expr.subject, indent + 1);
            for (int i = 0; i < node->when_expr.arms.count; i++)
                ast_print(node->when_expr.arms.items[i], indent + 1);
            break;
        case NODE_WHEN_ARM:
            printf("WhenArm(%s)\n", node->when_arm.pattern ? "" : "default");
            ast_print(node->when_arm.pattern, indent + 1);
            ast_print(node->when_arm.body,    indent + 1);
            break;
        case NODE_BINARY:
            printf("Binary(%d)\n", node->binary.op);
            ast_print(node->binary.left,  indent + 1);
            ast_print(node->binary.right, indent + 1);
            break;
        case NODE_UNARY:
            printf("Unary(%d)\n", node->unary.op);
            ast_print(node->unary.operand, indent + 1);
            break;
        case NODE_FIELD:
            printf("Field(.%s)\n", node->field.name);
            ast_print(node->field.target, indent + 1);
            break;
        case NODE_FIELD_PTR:
            printf("FieldPtr(->%s)\n", node->field.name);
            ast_print(node->field.target, indent + 1);
            break;
        case NODE_SUBSCRIPT:
            printf("Subscript\n");
            ast_print(node->subscript.target, indent + 1);
            ast_print(node->subscript.index,  indent + 1);
            break;
        case NODE_DEREF:
            printf("Deref\n");
            ast_print(node->deref.target, indent + 1);
            break;
        case NODE_STR_LIT:   printf("StrLit(\"%s\")\n", node->str_lit.value);  break;
        case NODE_INT_LIT:   printf("IntLit(%lld)\n",   node->int_lit.value);  break;
        case NODE_FLOAT_LIT: printf("FloatLit(%g)\n",   node->float_lit.value);break;
        case NODE_CHAR_LIT:  printf("CharLit('%c')\n",  node->char_lit.value); break;
        case NODE_BOOL_LIT:  printf("BoolLit(%s)\n",    node->bool_lit.value ? "true" : "false"); break;
        case NODE_IDENT:     printf("Ident(%s)\n",      node->ident.name);     break;
        default:             printf("Node(%d)\n",       node->kind);            break;
    }
}
