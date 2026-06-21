// co-builder-core/parser.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

// --- Tipos de nodos AST ---
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_BLOCK,
    NODE_VARDECL,
    NODE_ASSIGN,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_CALL,
    NODE_BINOP,
    NODE_UNARY,
    NODE_IDENT,
    NODE_NUMBER,
    NODE_STRING,
} NodeType;

typedef struct Node {
    NodeType type;
    char value[256];
    struct Node *left;
    struct Node *right;
    struct Node *cond;
    struct Node *body;
    struct Node *alt;
    struct Node **children;
    int child_count;
} Node;

typedef struct {
    Lexer lexer;
    Token current;
} Parser;

// --- Helpers ---
static Node *new_node(NodeType type, const char *value) {
    Node *n = calloc(1, sizeof(Node));
    n->type = type;
    if (value) strncpy(n->value, value, 255);
    return n;
}

static void advance(Parser *p) {
    p->current = lexer_next(&p->lexer);
}

static Token expect(Parser *p, TokenType t) {
    Token tok = p->current;
    if (tok.type != t) {
        fprintf(stderr, "Error linea %d: token inesperado '%s'\n", tok.line, tok.value);
        exit(1);
    }
    advance(p);
    return tok;
}

// --- Declaraciones forward ---
static Node *parse_expr(Parser *p);
static Node *parse_stmt(Parser *p);
static Node *parse_block(Parser *p);

// --- Expresiones ---
static Node *parse_primary(Parser *p) {
    Token t = p->current;

    if (t.type == TOK_NUMBER) {
        advance(p);
        return new_node(NODE_NUMBER, t.value);
    }
    if (t.type == TOK_STRING) {
        advance(p);
        return new_node(NODE_STRING, t.value);
    }
    if (t.type == TOK_IDENT) {
        advance(p);
        // llamada a funcion
        if (p->current.type == TOK_LPAR) {
            Node *call = new_node(NODE_CALL, t.value);
            call->children = malloc(sizeof(Node*) * 16);
            advance(p); // (
            while (p->current.type != TOK_RPAR) {
                call->children[call->child_count++] = parse_expr(p);
                if (p->current.type == TOK_COMMA) advance(p);
            }
            expect(p, TOK_RPAR);
            return call;
        }
        return new_node(NODE_IDENT, t.value);
    }
    if (t.type == TOK_LPAR) {
        advance(p);
        Node *n = parse_expr(p);
        expect(p, TOK_RPAR);
        return n;
    }
    if (t.type == TOK_MINUS) {
        advance(p);
        Node *n = new_node(NODE_UNARY, "-");
        n->left = parse_primary(p);
        return n;
    }

    fprintf(stderr, "Error linea %d: expresion inesperada '%s'\n", t.line, t.value);
    exit(1);
}

static Node *parse_term(Parser *p) {
    Node *left = parse_primary(p);
    while (p->current.type == TOK_STAR || p->current.type == TOK_SLASH) {
        char op[2] = {p->current.value[0], '\0'};
        advance(p);
        Node *n = new_node(NODE_BINOP, op);
        n->left = left;
        n->right = parse_primary(p);
        left = n;
    }
    return left;
}

static Node *parse_expr(Parser *p) {
    Node *left = parse_term(p);
    while (p->current.type == TOK_PLUS || p->current.type == TOK_MINUS ||
           p->current.type == TOK_EQEQ || p->current.type == TOK_NEQ  ||
           p->current.type == TOK_LT   || p->current.type == TOK_GT) {
        char op[3];
        strncpy(op, p->current.value, 2); op[2] = '\0';
        advance(p);
        Node *n = new_node(NODE_BINOP, op);
        n->left = left;
        n->right = parse_term(p);
        left = n;
    }
    return left;
}

// --- Statements ---
static Node *parse_stmt(Parser *p) {
    Token t = p->current;

    // return
    if (t.type == TOK_RETURN) {
        advance(p);
        Node *n = new_node(NODE_RETURN, "return");
        n->left = parse_expr(p);
        expect(p, TOK_SEMI);
        return n;
    }

    // if
    if (t.type == TOK_IF) {
        advance(p);
        Node *n = new_node(NODE_IF, "if");
        expect(p, TOK_LPAR);
        n->cond = parse_expr(p);
        expect(p, TOK_RPAR);
        n->body = parse_block(p);
        if (p->current.type == TOK_ELSE) {
            advance(p);
            n->alt = parse_block(p);
        }
        return n;
    }

    // while
    if (t.type == TOK_WHILE) {
        advance(p);
        Node *n = new_node(NODE_WHILE, "while");
        expect(p, TOK_LPAR);
        n->cond = parse_expr(p);
        expect(p, TOK_RPAR);
        n->body = parse_block(p);
        return n;
    }

    // declaracion de variable: int x = ...;
    if (t.type == TOK_INT || t.type == TOK_CHAR || t.type == TOK_VOID) {
        advance(p);
        Token name = expect(p, TOK_IDENT);
        Node *n = new_node(NODE_VARDECL, name.value);
        if (p->current.type == TOK_EQ) {
            advance(p);
            n->left = parse_expr(p);
        }
        expect(p, TOK_SEMI);
        return n;
    }

    // asignacion o llamada: x = ...; o func();
    if (t.type == TOK_IDENT) {
        advance(p);
        if (p->current.type == TOK_EQ) {
            advance(p);
            Node *n = new_node(NODE_ASSIGN, t.value);
            n->left = parse_expr(p);
            expect(p, TOK_SEMI);
            return n;
        }
        // llamada a funcion como statement
        if (p->current.type == TOK_LPAR) {
            Node *call = new_node(NODE_CALL, t.value);
            call->children = malloc(sizeof(Node*) * 16);
            advance(p);
            while (p->current.type != TOK_RPAR) {
                call->children[call->child_count++] = parse_expr(p);
                if (p->current.type == TOK_COMMA) advance(p);
            }
            expect(p, TOK_RPAR);
            expect(p, TOK_SEMI);
            return call;
        }
    }

    fprintf(stderr, "Error linea %d: statement inesperado '%s'\n", t.line, t.value);
    exit(1);
}

static Node *parse_block(Parser *p) {
    Node *block = new_node(NODE_BLOCK, "block");
    block->children = malloc(sizeof(Node*) * 64);
    expect(p, TOK_LBRACE);
    while (p->current.type != TOK_RBRACE && p->current.type != TOK_EOF) {
        block->children[block->child_count++] = parse_stmt(p);
    }
    expect(p, TOK_RBRACE);
    return block;
}

// --- Funcion ---
static Node *parse_function(Parser *p) {
    // tipo de retorno
    advance(p); // int/void/char
    Token name = expect(p, TOK_IDENT);
    Node *fn = new_node(NODE_FUNCTION, name.value);
    expect(p, TOK_LPAR);
    // parametros (por ahora los ignoramos)
    while (p->current.type != TOK_RPAR) advance(p);
    expect(p, TOK_RPAR);
    fn->body = parse_block(p);
    return fn;
}

// --- Entry point ---
Node *parse(const char *src) {
    Parser p;
    lexer_init(&p.lexer, src);
    advance(&p);

    Node *program = new_node(NODE_PROGRAM, "program");
    program->children = malloc(sizeof(Node*) * 64);

    // ignorar #include y similares
    while (p.current.type != TOK_EOF) {
        if (p.current.type == TOK_INT || p.current.type == TOK_VOID || p.current.type == TOK_CHAR) {
            program->children[program->child_count++] = parse_function(&p);
        } else {
            advance(&p); // saltar directivas
        }
    }
    return program;
}
