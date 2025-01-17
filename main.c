#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessador.h"
#include "montador.h"


int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.asm|arquivo.pre>\n", argv[0]);
        exit(1);
    }

    // Detect extension
    char *input_file = argv[1];
    char *dot = strrchr(input_file, '.');
    if (!dot) {
        fprintf(stderr, "Erro: arquivo sem extensão.\n");
        exit(1);
    }

    // Build output filename
    char output_file[256];
    strcpy(output_file, input_file);

    if (strcasecmp(dot, ".asm") == 0) {
        // Replace .asm with .pre
        strcpy(strrchr(output_file, '.'), ".pre");

        // Call your preprocessor
        preprocess_file(input_file, output_file);
        printf("Preprocessamento concluído. Arquivo gerado: %s\n", output_file);
    }
    else if (strcasecmp(dot, ".pre") == 0) {
        // Replace .pre with .obj
        strcpy(strrchr(output_file, '.'), ".obj");

        // Call assembler
        montar_programa(input_file, output_file);
        printf("Montagem concluída. Saída: %s\n", output_file);

    }
    else {
        fprintf(stderr, "Extensão não reconhecida ('%s'). Use .asm ou .pre.\n", dot);
        exit(1);
    }

    return 0;
}