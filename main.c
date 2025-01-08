#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessor.h"
#include "montador.h"
#include "ligador.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Uso: %s <opção> <arquivo1> <arquivo2> <arquivo_saida>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "montar") == 0) {
        preprocess_file(argv[2], "preprocessed.asm");
        montar_programa("preprocessed.asm", argv[3]);
    } else if (strcmp(argv[1], "ligar") == 0) {
        ligar_programa(argv[2], argv[3], argv[4]);
    } else {
        printf("Opção inválida. Use 'montar' ou 'ligar'.\n");
    }

    return 0;
}
