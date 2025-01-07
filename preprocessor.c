#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"

#define MAX_LINES 1000

void preprocess_line(char *line) {
    char *comment = strchr(line, ';');
    if (comment) {
        *comment = '\0';
    }

    char *start = line;
    while (isspace((unsigned char)*start)) start++;

    char *end = line + strlen(line) - 1;
    while (end > start && isspace((unsigned char)*end)) *end-- = '\0';

    memmove(line, start, strlen(start) + 1);

    for (char *p = line; *p; p++) {
        *p = toupper((unsigned char)*p);
    }
}

void preprocess_file(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "r");
    if (!input_file) {
        perror("Erro ao abrir o arquivo de entrada");
        exit(1);
    }

    FILE *output_file = fopen(output_filename, "w");
    if (!output_file) {
        perror("Erro ao criar o arquivo de saída");
        fclose(input_file);
        exit(1);
    }

    char line[MAX_LINES][256];
    int line_count = 0;

    while (fgets(line[line_count], sizeof(line[line_count]), input_file)) {
        preprocess_line(line[line_count]);
        if (strlen(line[line_count]) > 0) {
            fprintf(output_file, "%s\n", line[line_count]);
            line_count++;
        }
    }

    fclose(input_file);
    fclose(output_file);

    printf("Pré-processamento concluído. Arquivo salvo como: %s\n", output_filename);
}
