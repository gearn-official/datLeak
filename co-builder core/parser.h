// co-builder-core/parser.h
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

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

Node *parse(const char *src);

#endif
