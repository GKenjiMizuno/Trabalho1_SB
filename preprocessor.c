#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"

#define MAX_LINES 1000
#define MAX_MACROS 100
#define MAX_MACRO_LINES 50

typedef struct {
    char name[50];               // Nome da macro
    char lines[MAX_MACRO_LINES][256]; // Corpo da macro
    int line_count;              // Número de linhas no corpo
} Macro;

Macro macros[MAX_MACROS];
int macro_count = 0;

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

int is_equ_or_if(const char *line) {
    // Verifica se a linha contém EQU ou IF
    if (strstr(line, "EQU") || strstr(line, "IF")) {
        return 1;
    }
    return 0;
}

int find_macro(const char *name) {
    for (int i = 0; i < macro_count; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
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

    char line[256];
    int inside_macro = 0;  // Indica se estamos dentro da definição de uma macro
    Macro current_macro;   // Macro sendo definida atualmente

    while (fgets(line, sizeof(line), input_file)) {
        preprocess_line(line);

        if (strlen(line) == 0) {
            continue; // Ignorar linhas vazias
        }

        if (is_equ_or_if(line)) {
            continue; // Ignorar EQU e IF
        }

        // Verificar início de uma macro
        if (strncmp(line, "MACRO", 5) == 0) {
            inside_macro = 1;
            current_macro.line_count = 0;
            char *macro_name = strtok(line + 5, " ");
            if (!macro_name) {
                fprintf(stderr, "Erro: Nome de macro ausente após 'MACRO'\n");
                exit(1);
            }
            strcpy(current_macro.name, macro_name);
            continue;
        }

        // Verificar fim de uma macro
        if (inside_macro && strncmp(line, "ENDMACRO", 8) == 0) {
            inside_macro = 0;
            if (macro_count >= MAX_MACROS) {
                fprintf(stderr, "Erro: Número máximo de macros excedido\n");
                exit(1);
            }
            macros[macro_count++] = current_macro; // Salvar macro
            continue;
        }

        // Armazenar corpo da macro
        if (inside_macro) {
            if (current_macro.line_count >= MAX_MACRO_LINES) {
                fprintf(stderr, "Erro: Número máximo de linhas na macro excedido\n");
                exit(1);
            }
            strcpy(current_macro.lines[current_macro.line_count++], line);
            continue;
        }

        // Expandir chamadas de macros
        int macro_index = find_macro(line);
        if (macro_index != -1) {
            for (int i = 0; i < macros[macro_index].line_count; i++) {
                fprintf(output_file, "%s\n", macros[macro_index].lines[i]);
            }
            continue;
        }

        // Escrever linha normal no arquivo de saída
        fprintf(output_file, "%s\n", line);
    }

    fclose(input_file);
    fclose(output_file);

    printf("Pré-processamento concluído. Arquivo salvo como: %s\n", output_filename);
}
