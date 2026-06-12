/*
 * datLeak - C VM / Interpreter
 * v0.1 by gearn
 * MIT License
 *
 * Supports: printf, int variables, basic arithmetic, return
 * Usage:  ./datleak file.c
 *         ./datleak          (interactive mode)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
   TOKENS
   ============================================================ */

typedef enum {
    TK_EOF = 0,
    TK_IDENT,      /* identifier or keyword  */
    TK_INT_LIT,    /* integer number         */
    TK_STR_LIT,    /* "string"               */
    TK_LPAREN,     /* (  */
    TK_RPAREN,     /* )  */
    TK_LBRACE,     /* {  */
    TK_RBRACE,     /* }  */
    TK_SEMI,       /* ;  */
    TK_COMMA,      /* ,  */
    TK_EQ,         /* =  */
    TK_PLUS,       /* +  */
    TK_MINUS,      /* -  */
    TK_STAR,       /* *  */
    TK_SLASH,      /* /  */
    TK_HASH,       /* # (preprocessor directive) */
    TK_UNKNOWN
} TkType;

typedef struct {
    TkType type;
    char   val[512];
    int    ival;
} Token;

/* ============================================================
   LEXER
   ============================================================ */

typedef struct {
    const char *src;
    int         pos;
    int         len;
} Lexer;

static void lex_init(Lexer *l, const char *src) {
    l->src = src;
    l->pos = 0;
    l->len = (int)strlen(src);
}

static void lex_skip(Lexer *l) {
    while (l->pos < l->len) {
        char c = l->src[l->pos];
        /* whitespace */
        if (isspace(c)) { l->pos++; continue; }
        /* line comment */
        if (c == '/' && l->pos+1 < l->len && l->src[l->pos+1] == '/') {
            while (l->pos < l->len && l->src[l->pos] != '\n') l->pos++;
            continue;
        }
        /* block comment */
        if (c == '/' && l->pos+1 < l->len && l->src[l->pos+1] == '*') {
            l->pos += 2;
            while (l->pos+1 < l->len &&
                   !(l->src[l->pos] == '*' && l->src[l->pos+1] == '/'))
                l->pos++;
            l->pos += 2;
            continue;
        }
        break;
    }
}

static Token lex_next(Lexer *l) {
    Token t;
    memset(&t, 0, sizeof(t));
    lex_skip(l);

    if (l->pos >= l->len) { t.type = TK_EOF; return t; }

    char c = l->src[l->pos];

    /* preprocessor directive: skip the entire line */
    if (c == '#') {
        while (l->pos < l->len && l->src[l->pos] != '\n') l->pos++;
        t.type = TK_HASH;
        return t;
    }

    /* identifier */
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (l->pos < l->len && (isalnum(l->src[l->pos]) || l->src[l->pos] == '_')) {
            if (i < 511) t.val[i++] = l->src[l->pos];
            l->pos++;
        }
        t.val[i] = '\0';
        t.type = TK_IDENT;
        return t;
    }

    /* integer literal */
    if (isdigit(c)) {
        int v = 0, i = 0;
        while (l->pos < l->len && isdigit(l->src[l->pos])) {
            v = v * 10 + (l->src[l->pos] - '0');
            if (i < 511) t.val[i++] = l->src[l->pos];
            l->pos++;
        }
        t.val[i] = '\0';
        t.ival = v;
        t.type = TK_INT_LIT;
        return t;
    }

    /* string literal */
    if (c == '"') {
        l->pos++;
        int i = 0;
        while (l->pos < l->len && l->src[l->pos] != '"') {
            if (l->src[l->pos] == '\\' && l->pos+1 < l->len) {
                l->pos++;
                char esc = l->src[l->pos++];
                char e;
                switch (esc) {
                    case 'n':  e = '\n'; break;
                    case 't':  e = '\t'; break;
                    case 'r':  e = '\r'; break;
                    case '\\': e = '\\'; break;
                    case '"':  e = '"';  break;
                    case '0':  e = '\0'; break;
                    default:   e = esc;  break;
                }
                if (i < 511) t.val[i++] = e;
            } else {
                if (i < 511) t.val[i++] = l->src[l->pos++];
            }
        }
        t.val[i] = '\0';
        if (l->pos < l->len) l->pos++; /* skip closing " */
        t.type = TK_STR_LIT;
        return t;
    }

    /* single character tokens */
    t.val[0] = c; t.val[1] = '\0';
    l->pos++;
    switch (c) {
        case '(': t.type = TK_LPAREN;  break;
        case ')': t.type = TK_RPAREN;  break;
        case '{': t.type = TK_LBRACE;  break;
        case '}': t.type = TK_RBRACE;  break;
        case ';': t.type = TK_SEMI;    break;
        case ',': t.type = TK_COMMA;   break;
        case '=': t.type = TK_EQ;      break;
        case '+': t.type = TK_PLUS;    break;
        case '-': t.type = TK_MINUS;   break;
        case '*': t.type = TK_STAR;    break;
        case '/': t.type = TK_SLASH;   break;
        default:  t.type = TK_UNKNOWN; break;
    }
    return t;
}

/* ============================================================
   VARIABLE STORE
   ============================================================ */

#define MAX_VARS 256

typedef struct {
    char name[64];
    int  ival;
} Var;

static Var vars[MAX_VARS];
static int var_count = 0;

static Var *find_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i].name, name) == 0) return &vars[i];
    return NULL;
}

static void set_var(const char *name, int val) {
    Var *v = find_var(name);
    if (!v) {
        if (var_count >= MAX_VARS) {
            fprintf(stderr, "[datLeak] Error: too many variables\n");
            exit(1);
        }
        v = &vars[var_count++];
        strncpy(v->name, name, 63);
    }
    v->ival = val;
}

/* ============================================================
   PARSER / INTERPRETER
   ============================================================ */

typedef struct {
    Lexer lex;
    Token cur;
    Token peek;
} Parser;

static void p_advance(Parser *p) {
    p->cur  = p->peek;
    p->peek = lex_next(&p->lex);
}

static void p_init(Parser *p, const char *src) {
    lex_init(&p->lex, src);
    p->peek = lex_next(&p->lex);
    p_advance(p);
}

static int p_expect(Parser *p, TkType t) {
    if (p->cur.type != t) {
        fprintf(stderr, "[datLeak] Syntax error: expected token %d, got '%s'\n",
                t, p->cur.val);
        return 0;
    }
    p_advance(p);
    return 1;
}

/* --- Expression evaluation (with precedence) --- */

static int eval_expr(Parser *p); /* forward declaration */

static int eval_primary(Parser *p) {
    /* integer literal */
    if (p->cur.type == TK_INT_LIT) {
        int v = p->cur.ival;
        p_advance(p);
        return v;
    }
    /* variable */
    if (p->cur.type == TK_IDENT) {
        Var *v = find_var(p->cur.val);
        p_advance(p);
        return v ? v->ival : 0;
    }
    /* parenthesized expression */
    if (p->cur.type == TK_LPAREN) {
        p_advance(p);
        int v = eval_expr(p);
        p_expect(p, TK_RPAREN);
        return v;
    }
    /* unary negation */
    if (p->cur.type == TK_MINUS) {
        p_advance(p);
        return -eval_primary(p);
    }
    p_advance(p);
    return 0;
}

static int eval_term(Parser *p) {
    int v = eval_primary(p);
    while (p->cur.type == TK_STAR || p->cur.type == TK_SLASH) {
        TkType op = p->cur.type;
        p_advance(p);
        int r = eval_primary(p);
        if (op == TK_STAR) v *= r;
        else if (r != 0)   v /= r;
    }
    return v;
}

static int eval_expr(Parser *p) {
    int v = eval_term(p);
    while (p->cur.type == TK_PLUS || p->cur.type == TK_MINUS) {
        TkType op = p->cur.type;
        p_advance(p);
        int r = eval_term(p);
        v = (op == TK_PLUS) ? v + r : v - r;
    }
    return v;
}

/* --- printf execution --- */

static void exec_printf(Parser *p) {
    p_expect(p, TK_LPAREN);

    if (p->cur.type != TK_STR_LIT) {
        fprintf(stderr, "[datLeak] printf: expected string literal\n");
        while (p->cur.type != TK_SEMI && p->cur.type != TK_EOF) p_advance(p);
        return;
    }

    char fmt[512];
    strncpy(fmt, p->cur.val, 511);
    p_advance(p);

    /* collect arguments */
    int  iargs[32];
    char sargs[32][512];
    int  atypes[32]; /* 0 = int, 1 = string */
    int  argc2 = 0;

    while (p->cur.type == TK_COMMA && argc2 < 32) {
        p_advance(p);
        if (p->cur.type == TK_STR_LIT) {
            strncpy(sargs[argc2], p->cur.val, 511);
            atypes[argc2++] = 1;
            p_advance(p);
        } else {
            iargs[argc2] = eval_expr(p);
            atypes[argc2++] = 0;
        }
    }

    p_expect(p, TK_RPAREN);
    p_expect(p, TK_SEMI);

    /* print according to format string */
    int ai = 0;
    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] == '%' && fmt[i+1]) {
            i++;
            switch (fmt[i]) {
                case 'd': case 'i':
                    if (ai < argc2 && atypes[ai] == 0) printf("%d", iargs[ai++]);
                    else ai++;
                    break;
                case 's':
                    if (ai < argc2 && atypes[ai] == 1) printf("%s", sargs[ai++]);
                    else ai++;
                    break;
                case 'c':
                    if (ai < argc2 && atypes[ai] == 0) printf("%c", (char)iargs[ai++]);
                    else ai++;
                    break;
                case '%':
                    putchar('%');
                    break;
                default:
                    putchar('%');
                    putchar(fmt[i]);
                    break;
            }
        } else {
            putchar(fmt[i]);
        }
    }
}

/* --- Statement execution --- */

static void exec_block(Parser *p);

static void exec_statement(Parser *p) {
    /* preprocessor directives already consumed by lexer */
    if (p->cur.type == TK_HASH) { p_advance(p); return; }

    /* block */
    if (p->cur.type == TK_LBRACE) { exec_block(p); return; }

    if (p->cur.type == TK_IDENT) {
        /* variable declaration: int / long / char / short / unsigned */
        if (!strcmp(p->cur.val,"int")    || !strcmp(p->cur.val,"long") ||
            !strcmp(p->cur.val,"char")   || !strcmp(p->cur.val,"short") ||
            !strcmp(p->cur.val,"unsigned")) {

            /* skip type (could be 'unsigned int' etc) */
            while (p->cur.type == TK_IDENT &&
                   (!strcmp(p->cur.val,"int")      || !strcmp(p->cur.val,"long") ||
                    !strcmp(p->cur.val,"char")     || !strcmp(p->cur.val,"short") ||
                    !strcmp(p->cur.val,"unsigned")))
                p_advance(p);

            if (p->cur.type != TK_IDENT) {
                while (p->cur.type != TK_SEMI && p->cur.type != TK_EOF) p_advance(p);
                if (p->cur.type == TK_SEMI) p_advance(p);
                return;
            }
            char vname[64];
            strncpy(vname, p->cur.val, 63);
            p_advance(p);

            int val = 0;
            if (p->cur.type == TK_EQ) { p_advance(p); val = eval_expr(p); }
            set_var(vname, val);
            p_expect(p, TK_SEMI);
            return;
        }

        char name[64];
        strncpy(name, p->cur.val, 63);
        p_advance(p);

        /* printf */
        if (!strcmp(name, "printf") && p->cur.type == TK_LPAREN) {
            exec_printf(p);
            return;
        }

        /* return */
        if (!strcmp(name, "return")) {
            if (p->cur.type != TK_SEMI) eval_expr(p);
            p_expect(p, TK_SEMI);
            return;
        }

        /* assignment: x = expr; */
        if (p->cur.type == TK_EQ) {
            p_advance(p);
            set_var(name, eval_expr(p));
            p_expect(p, TK_SEMI);
            return;
        }

        /* unknown call or expression: skip */
        while (p->cur.type != TK_SEMI && p->cur.type != TK_EOF) p_advance(p);
        if (p->cur.type == TK_SEMI) p_advance(p);
        return;
    }

    /* anything else: skip */
    p_advance(p);
}

static void exec_block(Parser *p) {
    p_expect(p, TK_LBRACE);
    while (p->cur.type != TK_RBRACE && p->cur.type != TK_EOF)
        exec_statement(p);
    p_expect(p, TK_RBRACE);
}

/* Find and execute main() */
static void exec_program(Parser *p) {
    int found = 0;
    while (p->cur.type != TK_EOF) {
        if (p->cur.type == TK_IDENT && !strcmp(p->cur.val, "main")) {
            p_advance(p);
            if (p->cur.type == TK_LPAREN) {
                p_advance(p);
                while (p->cur.type != TK_RPAREN && p->cur.type != TK_EOF) p_advance(p);
                p_advance(p); /* skip ) */
                if (p->cur.type == TK_LBRACE) {
                    found = 1;
                    exec_block(p);
                    break;
                }
            }
        } else {
            p_advance(p);
        }
    }
    if (!found)
        fprintf(stderr, "[datLeak] Error: main() not found\n");
}

/* ============================================================
   MAIN CLI
   ============================================================ */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char *argv[]) {
    printf("datLeak C VM v0.1 - by gearn\n");
    printf("==============================\n");

    if (argc < 2) {
        /* interactive mode */
        printf("Interactive mode. Write your C code and press Ctrl+D when done.\n\n");
        char *buf = NULL;
        size_t total = 0;
        char line[1024];
        while (fgets(line, sizeof(line), stdin)) {
            size_t ln = strlen(line);
            buf = (char*)realloc(buf, total + ln + 1);
            memcpy(buf + total, line, ln);
            total += ln;
            buf[total] = '\0';
        }
        if (buf) {
            printf("\n--- Output ---\n");
            Parser p;
            p_init(&p, buf);
            exec_program(&p);
            free(buf);
        }
    } else {
        /* file mode */
        char *src = read_file(argv[1]);
        if (src) {
            printf("Running: %s\n", argv[1]);
            printf("--- Output ---\n");
            Parser p;
            p_init(&p, src);
            exec_program(&p);
            free(src);
        }
    }

    return 0;
}
