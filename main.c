#include <stdio.h>
#include <stdlib.h>
#include "preprocessor.h"
#include "montador.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <arquivo.asm> <arquivo.obj>\n", argv[0]);
        return 1;
    }

    char preprocessed_filename[256];
    snprintf(preprocessed_filename, sizeof(preprocessed_filename), "%s_pre.asm", argv[1]);

    printf("Iniciando pré-processamento...\n");
    preprocess_file(argv[1], preprocessed_filename);

    printf("Iniciando montagem em única passagem...\n");
    montar_programa(preprocessed_filename, argv[2]);

    printf("Processo concluído. Arquivo de saída: %s\n", argv[2]);
    return 0;
}
