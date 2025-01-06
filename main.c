#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessor.h"
#include "montador.h"

#define MAX_LINES 1000

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Uso: %s <arquivo.asm>\n", argv[0]);
        return 1;
    }

    // Pré-processamento
    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    char pre_output_filename[256];
    snprintf(pre_output_filename, sizeof(pre_output_filename), "%s_pre.asm", strtok(strdup(argv[1]), "."));
    FILE *pre_output_file = fopen(pre_output_filename, "w");
    if (pre_output_file == NULL) {
        perror("Erro ao criar o arquivo de saída do pré-processamento");
        fclose(input_file);
        return 1;
    }

    char lines[MAX_LINES][256];
    int line_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), input_file)) {
        preprocess_line(line);

        if (strstr(line, "EQU") || strstr(line, "IF")) {
            preprocess_equ_if(line);
        }

        remove_extra_spaces(line);
        validate_copy(line);
        validate_const(line);

        if (strlen(line) > 0) {
            strncpy(lines[line_count++], line, sizeof(line));
        }
    }

    fclose(input_file);
    associate_labels(lines, &line_count);
    reorder_sections(lines, line_count, pre_output_file);
    fclose(pre_output_file);

    printf("Pré-processamento concluído. Arquivo salvo como: %s\n", pre_output_filename);

    // Montador
    char montador_output_filename[256];
    snprintf(montador_output_filename, sizeof(montador_output_filename), "%s.obj", strtok(strdup(argv[1]), "."));
    montar_programa(pre_output_filename, montador_output_filename);

    printf("Montagem concluída. Arquivo salvo como: %s\n", montador_output_filename);

    return 0;
}
