#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 1000

// Função para remover espaços extras e comentários
void preprocess_line(char *line) {
    char *comment = strchr(line, ';'); // Procura pelo início do comentário
    if (comment) {
        *comment = '\0'; // Remove o comentário da linha
    }

    // Remove espaços no início e final da linha
    char *start = line;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    char *end = line + strlen(line) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    // Move a linha processada para o início do buffer
    memmove(line, start, strlen(start) + 1);

    // Converte para maiúsculas
    for (char *p = line; *p; p++) {
        *p = toupper((unsigned char)*p);
    }
}

// Função para separar e reordenar as seções TEXT e DATA
void reorder_sections(char lines[MAX_LINES][256], int line_count, FILE *output_file) {
    char text_section[MAX_LINES][256];
    char data_section[MAX_LINES][256];
    int text_count = 0, data_count = 0;

    int current_section = 0; // 0 = Nenhuma, 1 = TEXT, 2 = DATA

    for (int i = 0; i < line_count; i++) {
        if (strcasecmp(lines[i], "SECTION TEXT") == 0) {
            current_section = 1;
            continue;
        } else if (strcasecmp(lines[i], "SECTION DATA") == 0) {
            current_section = 2;
            continue;
        }

        if (current_section == 1) {
            strncpy(text_section[text_count++], lines[i], sizeof(lines[i]));
        } else if (current_section == 2) {
            strncpy(data_section[data_count++], lines[i], sizeof(lines[i]));
        }
    }

    // Escreve a seção TEXT primeiro
    fprintf(output_file, "SECTION TEXT\n");
    for (int i = 0; i < text_count; i++) {
        if (strchr(text_section[i], ':')) { // Rótulo na linha
            fprintf(output_file, "%s\n", text_section[i]);
        } else {
            fprintf(output_file, "    %s\n", text_section[i]);
        }
    }

    // Escreve a seção DATA por último
    fprintf(output_file, "SECTION DATA\n");
    for (int i = 0; i < data_count; i++) {
        fprintf(output_file, "%s\n", data_section[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo.asm>\n", argv[0]);
        return 1;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    // Arquivo de saída pré-processado
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "%s.pre", argv[1]);
    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        perror("Erro ao criar o arquivo de saída");
        fclose(input_file);
        return 1;
    }

    char lines[MAX_LINES][256];
    int line_count = 0;

    // Ler o arquivo linha por linha e pré-processar
    char line[256]; // Buffer para armazenar cada linha
    while (fgets(line, sizeof(line), input_file)) {
        preprocess_line(line);
        if (strlen(line) > 0) { // Ignora linhas vazias após o processamento
            strncpy(lines[line_count++], line, sizeof(line));
        }
    }

    fclose(input_file);

    // Reordenar e escrever as seções
    reorder_sections(lines, line_count, output_file);

    fclose(output_file);

    printf("Pré-processamento concluído. Arquivo salvo como: %s\n", output_filename);
    return 0;
}
