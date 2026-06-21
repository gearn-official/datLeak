// co-builder-core/codegen.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

typedef struct {
    char name[256];
    int offset; // offset en el stack
} Var;

typedef struct {
    FILE *out;
    Var vars[256];
    int var_count;
    int stack_offset;
    int label_count;
} Codegen;

static int new_label(Codegen *cg) {
    return cg->label_count++;
}

static int find_var(Codegen *cg, const char *name) {
    for (int i = 0; i < cg->var_count; i++) {
        if (strcmp(cg->vars[i].name, name) == 0)
            return cg->vars[i].offset;
    }
    fprintf(stderr, "Error: variable '%s' no declarada\n", name);
    exit(1);
}

static void emit(Codegen *cg, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(cg->out, fmt, args);
    va_end(args);
    fprintf(cg->out, "\n");
}

static void gen_expr(Codegen *cg, Node *n);
static void gen_stmt(Codegen *cg, Node *n);

static void gen_expr(Codegen *cg, Node *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_NUMBER:
            emit(cg, "    mov eax, %s", n->value);
            break;

        case NODE_STRING:
            // los strings se manejan en la seccion de datos
            emit(cg, "    ; string literal '%s'", n->value);
            break;

        case NODE_IDENT: {
            int off = find_var(cg, n->value);
            emit(cg, "    mov eax, [ebp%d]", off);
            break;
        }

        case NODE_UNARY:
            gen_expr(cg, n->left);
            emit(cg, "    neg eax");
            break;

        case NODE_BINOP:
            gen_expr(cg, n->left);
            emit(cg, "    push eax");
            gen_expr(cg, n->right);
            emit(cg, "    pop ecx");
            if (strcmp(n->value, "+") == 0)
                emit(cg, "    add eax, ecx");
            else if (strcmp(n->value, "-") == 0) {
                emit(cg, "    sub ecx, eax");
                emit(cg, "    mov eax, ecx");
            } else if (strcmp(n->value, "*") == 0)
                emit(cg, "    imul eax, ecx");
            else if (strcmp(n->value, "/") == 0) {
                emit(cg, "    mov edx, ecx");
                emit(cg, "    mov ecx, eax");
                emit(cg, "    mov eax, edx");
                emit(cg, "    cdq");
                emit(cg, "    idiv ecx");
            } else if (strcmp(n->value, "==") == 0) {
                emit(cg, "    cmp ecx, eax");
                emit(cg, "    sete al");
                emit(cg, "    movzx eax, al");
            } else if (strcmp(n->value, "!=") == 0) {
                emit(cg, "    cmp ecx, eax");
                emit(cg, "    setne al");
                emit(cg, "    movzx eax, al");
            } else if (strcmp(n->value, "<") == 0) {
                emit(cg, "    cmp ecx, eax");
                emit(cg, "    setl al");
                emit(cg, "    movzx eax, al");
            } else if (strcmp(n->value, ">") == 0) {
                emit(cg, "    cmp ecx, eax");
                emit(cg, "    setg al");
                emit(cg, "    movzx eax, al");
            }
            break;

        case NODE_CALL:
            // argumentos en orden inverso
            for (int i = n->child_count - 1; i >= 0; i--) {
                gen_expr(cg, n->children[i]);
                emit(cg, "    push eax");
            }
            emit(cg, "    call _%s", n->value);
            if (n->child_count > 0)
                emit(cg, "    add esp, %d", n->child_count * 4);
            break;

        default:
            break;
    }
}

static void gen_stmt(Codegen *cg, Node *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_VARDECL: {
            cg->stack_offset -= 4;
            strncpy(cg->vars[cg->var_count].name, n->value, 255);
            cg->vars[cg->var_count].offset = cg->stack_offset;
            cg->var_count++;
            emit(cg, "    sub esp, 4");
            if (n->left) {
                gen_expr(cg, n->left);
                emit(cg, "    mov [ebp%d], eax", cg->stack_offset);
            }
            break;
        }

        case NODE_ASSIGN: {
            gen_expr(cg, n->left);
            int off = find_var(cg, n->value);
            emit(cg, "    mov [ebp%d], eax", off);
            break;
        }

        case NODE_RETURN:
            gen_expr(cg, n->left);
            emit(cg, "    mov esp, ebp");
            emit(cg, "    pop ebp");
            emit(cg, "    ret");
            break;

        case NODE_IF: {
            int lbl_else = new_label(cg);
            int lbl_end  = new_label(cg);
            gen_expr(cg, n->cond);
            emit(cg, "    cmp eax, 0");
            emit(cg, "    je .L%d", lbl_else);
            gen_stmt(cg, n->body);
            emit(cg, "    jmp .L%d", lbl_end);
            emit(cg, ".L%d:", lbl_else);
            if (n->alt) gen_stmt(cg, n->alt);
            emit(cg, ".L%d:", lbl_end);
            break;
        }

        case NODE_WHILE: {
            int lbl_start = new_label(cg);
            int lbl_end   = new_label(cg);
            emit(cg, ".L%d:", lbl_start);
            gen_expr(cg, n->cond);
            emit(cg, "    cmp eax, 0");
            emit(cg, "    je .L%d", lbl_end);
            gen_stmt(cg, n->body);
            emit(cg, "    jmp .L%d", lbl_start);
            emit(cg, ".L%d:", lbl_end);
            break;
        }

        case NODE_BLOCK:
            for (int i = 0; i < n->child_count; i++)
                gen_stmt(cg, n->children[i]);
            break;

        case NODE_CALL:
            gen_expr(cg, n);
            break;

        default:
            break;
    }
}

static void gen_function(Codegen *cg, Node *n) {
    cg->var_count = 0;
    cg->stack_offset = 0;

    emit(cg, "global _%s", n->value);
    emit(cg, "_%s:", n->value);
    emit(cg, "    push ebp");
    emit(cg, "    mov ebp, esp");

    gen_stmt(cg, n->body);

    // return 0 por defecto
    emit(cg, "    xor eax, eax");
    emit(cg, "    mov esp, ebp");
    emit(cg, "    pop ebp");
    emit(cg, "    ret");
}

void codegen(Node *program, const char *outfile) {
    Codegen cg;
    memset(&cg, 0, sizeof(cg));
    cg.out = fopen(outfile, "w");
    if (!cg.out) {
        fprintf(stderr, "Error: no se pudo abrir '%s'\n", outfile);
        exit(1);
    }

    emit(&cg, "section .text");
    emit(&cg, "extern _printf");
    emit(&cg, "");

    for (int i = 0; i < program->child_count; i++) {
        if (program->children[i]->type == NODE_FUNCTION)
            gen_function(&cg, program->children[i]);
    }

    fclose(cg.out);
    printf("co-builder: output -> %s\n", outfile);
}
