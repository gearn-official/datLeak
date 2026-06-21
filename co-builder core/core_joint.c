// co-builder-core/core_joint.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "co-builder: error: no se pudo abrir '%s'\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "co-builder v0.1 - part of datLeak\n");
        fprintf(stderr, "uso: co-builder <archivo.c> [-o salida.asm]\n");
        return 1;
    }

    const char *input = argv[1];
    const char *output = "output.asm";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output = argv[++i];
        }
    }

    printf("co-builder: compilando '%s'...\n", input);

    char *src = read_file(input);
    Node *ast = parse(src);
    codegen(ast, output);

    free(src);
    printf("co-builder: listo -> '%s'\n", output);
    return 0;
}
