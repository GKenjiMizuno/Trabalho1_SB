#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessor.h"
#include "montador.h"
//#include "ligador.h"

//int main(int argc, char *argv[]) {
//    if (argc < 4) {
//        printf("Uso: %s <opção> <arquivo1> <arquivo2> <arquivo_saida>\n", argv[0]);
//        return 1;
//    }
//
//    if (strcmp(argv[1], "montar") == 0) {
////        preprocess_file(argv[2], "preprocessed.asm");
////        preprocess_file("prog.asm", "prog-pre.asm");
//        montar_programa("progB-pre.asm", "progB.obj");
////        montar_programa("prog1-pre.asm", "prog1.obj");
////        montar_programa("prog-pre.asm", "prog.obj");
////    } else if (strcmp(argv[1], "ligar") == 0) {
////        ligar_programa(argv[2], argv[3], argv[4]);
//    } else {
//        printf("Opção inválida. Use 'montar' ou 'ligar'.\n");
//    }
//
//    return 0;
//}

int main(int argc, char *argv[])
{
//    if (argc != 2) {
//        fprintf(stderr, "Uso: %s <arquivo.asm|arquivo.pre>\n", argv[0]);
//        exit(1);
//    }

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
//        pre_process(input_file, output_file);
        printf("Preprocessamento concluído. Arquivo gerado: %s\n", output_file);
    }
    else if (strcasecmp(dot, ".pre") == 0) {
        // Replace .pre with .obj
        strcpy(strrchr(output_file, '.'), ".obj");

        // Call assembler
        montar_programa(input_file, output_file);
        // The assembler prints final messages itself
    }
    else {
        fprintf(stderr, "Extensão não reconhecida ('%s'). Use .asm ou .pre.\n", dot);
        exit(1);
    }

    return 0;
}