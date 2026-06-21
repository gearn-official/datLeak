// co-builder-core/lexer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOK_INT, TOK_CHAR, TOK_VOID, TOK_RETURN,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR,
    TOK_IDENT, TOK_NUMBER, TOK_STRING,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH,
    TOK_EQ, TOK_EQEQ, TOK_NEQ, TOK_LT, TOK_GT,
    TOK_LPAR, TOK_RPAR, TOK_LBRACE, TOK_RBRACE,
    TOK_SEMI, TOK_COMMA, TOK_AMP,
    TOK_EOF, TOK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int line;
} Token;

typedef struct {
    const char *src;
    int pos;
    int line;
} Lexer;

void lexer_init(Lexer *l, const char *src) {
    l->src = src;
    l->pos = 0;
    l->line = 1;
}

static char peek(Lexer *l) {
    return l->src[l->pos];
}

static char advance(Lexer *l) {
    char c = l->src[l->pos++];
    if (c == '\n') l->line++;
    return c;
}

static void skip_whitespace(Lexer *l) {
    while (isspace(peek(l))) advance(l);
}

static void skip_comment(Lexer *l) {
    if (peek(l) == '/') {
        advance(l); advance(l);
        while (peek(l) && peek(l) != '\n') advance(l);
    } else if (peek(l) == '*') {
        advance(l); advance(l);
        while (peek(l)) {
            if (peek(l) == '*' && l->src[l->pos+1] == '/') {
                advance(l); advance(l);
                break;
            }
            advance(l);
        }
    }
}

Token lexer_next(Lexer *l) {
    Token tok;
    tok.value[0] = '\0';
    tok.line = l->line;

    skip_whitespace(l);

    char c = peek(l);

    if (c == '\0') { tok.type = TOK_EOF; return tok; }

    // comentarios
    if (c == '/' && (l->src[l->pos+1] == '/' || l->src[l->pos+1] == '*')) {
        skip_comment(l);
        return lexer_next(l);
    }

    // numeros
    if (isdigit(c)) {
        int i = 0;
        while (isdigit(peek(l))) tok.value[i++] = advance(l);
        tok.value[i] = '\0';
        tok.type = TOK_NUMBER;
        return tok;
    }

    // strings
    if (c == '"') {
        advance(l);
        int i = 0;
        while (peek(l) && peek(l) != '"') tok.value[i++] = advance(l);
        advance(l);
        tok.value[i] = '\0';
        tok.type = TOK_STRING;
        return tok;
    }

    // keywords e identifiers
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (isalnum(peek(l)) || peek(l) == '_') tok.value[i++] = advance(l);
        tok.value[i] = '\0';

        if      (strcmp(tok.value, "int")    == 0) tok.type = TOK_INT;
        else if (strcmp(tok.value, "char")   == 0) tok.type = TOK_CHAR;
        else if (strcmp(tok.value, "void")   == 0) tok.type = TOK_VOID;
        else if (strcmp(tok.value, "return") == 0) tok.type = TOK_RETURN;
        else if (strcmp(tok.value, "if")     == 0) tok.type = TOK_IF;
        else if (strcmp(tok.value, "else")   == 0) tok.type = TOK_ELSE;
        else if (strcmp(tok.value, "while")  == 0) tok.type = TOK_WHILE;
        else if (strcmp(tok.value, "for")    == 0) tok.type = TOK_FOR;
        else tok.type = TOK_IDENT;
        return tok;
    }

    advance(l);
    tok.value[0] = c; tok.value[1] = '\0';

    switch (c) {
        case '+': tok.type = TOK_PLUS;   break;
        case '-': tok.type = TOK_MINUS;  break;
        case '*': tok.type = TOK_STAR;   break;
        case '/': tok.type = TOK_SLASH;  break;
        case '(': tok.type = TOK_LPAR;   break;
        case ')': tok.type = TOK_RPAR;   break;
        case '{': tok.type = TOK_LBRACE; break;
        case '}': tok.type = TOK_RBRACE; break;
        case ';': tok.type = TOK_SEMI;   break;
        case ',': tok.type = TOK_COMMA;  break;
        case '&': tok.type = TOK_AMP;    break;
        case '<': tok.type = TOK_LT;     break;
        case '>': tok.type = TOK_GT;     break;
        case '=':
            if (peek(l) == '=') { advance(l); tok.type = TOK_EQEQ; strcpy(tok.value, "=="); }
            else tok.type = TOK_EQ;
            break;
        case '!':
            if (peek(l) == '=') { advance(l); tok.type = TOK_NEQ; strcpy(tok.value, "!="); }
            else tok.type = TOK_UNKNOWN;
            break;
        default: tok.type = TOK_UNKNOWN; break;
    }
    return tok;
}
