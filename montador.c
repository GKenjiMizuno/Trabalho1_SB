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

// Função para validar a instrução COPY
void validate_copy(char *line) {
    if (strncmp(line, "COPY", 4) == 0) {
        char *comma = strchr(line, ',');
        if (!comma || *(comma + 1) == ' ') {
            fprintf(stderr, "Erro: COPY com argumentos mal formatados: %s\n", line);
            exit(1);
        }
    }
}

// Função para remover espaços extras dentro da linha
void remove_extra_spaces(char *line) {
    char temp[256];
    int j = 0;
    int in_space = 0;

    for (int i = 0; line[i] != '\0'; i++) {
        if (isspace((unsigned char)line[i])) {
            if (!in_space) {
                temp[j++] = ' ';
                in_space = 1;
            }
        } else {
            temp[j++] = line[i];
            in_space = 0;
        }
    }
    temp[j] = '\0';
    strcpy(line, temp);
}

// Função para validar a diretiva CONST
void validate_const(char *line) {
    if (strstr(line, "CONST")) {
        char *value = strstr(line, "CONST") + 6; // Pula "CONST " para pegar o valor
        while (isspace((unsigned char)*value)) {
            value++;
        }

        // Verifica se o valor é um decimal, negativo ou hexadecimal
        if (!isdigit(*value) && *value != '-' && strncmp(value, "0X", 2) != 0) {
            fprintf(stderr, "Erro: Valor inválido para CONST: %s\n", line);
            exit(1);
        }

        // Se for hexadecimal, verifica se segue o formato correto
        if (strncmp(value, "0X", 2) == 0) {
            value += 2;
            while (*value) {
                if (!isxdigit(*value)) { // Verifica apenas caracteres hexadecimais (0-9, A-F, a-f)
                    fprintf(stderr, "Erro: Valor hexadecimal inválido para CONST: %s\n", line);
                    exit(1);
                }
                value++;
            }
        } else {
            // Verifica se todos os caracteres de um decimal são dígitos
            if (*value == '-') {
                value++;
            }
            while (*value) {
                if (!isdigit(*value)) {
                    fprintf(stderr, "Erro: Valor decimal inválido para CONST: %s\n", line);
                    exit(1);
                }
                value++;
            }
        }
    }
}

// Função para remover comentários em EQU e IF
void preprocess_equ_if(char *line) {
    if (strstr(line, "EQU") || strstr(line, "IF")) {
        char *comment = strchr(line, ';'); // Localiza o comentário
        if (comment) {
            *comment = '\0'; // Remove o comentário
        }
        remove_extra_spaces(line); // Remove espaços extras
    }
}

// Função para associar rótulos à próxima instrução
void associate_labels(char lines[MAX_LINES][256], int *line_count) {
    for (int i = 0; i < *line_count - 1; i++) {
        char *colon = strchr(lines[i], ':');
        if (colon && strlen(colon + 1) == 0) { // Verifica se há só o rótulo na linha
            strcat(lines[i], " ");
            strcat(lines[i], lines[i + 1]); // Concatena com a próxima linha
            for (int j = i + 1; j < *line_count - 1; j++) {
                strcpy(lines[j], lines[j + 1]); // Move as linhas para cima
            }
            (*line_count)--;
        }
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

        if (strstr(line, "EQU") || strstr(line, "IF")) {
            preprocess_equ_if(line); // Remove comentários em EQU e IF
        }

        remove_extra_spaces(line); // Remove espaços extras
        validate_copy(line); // Valida a instrução COPY
        validate_const(line); // Valida a diretiva CONST

        if (strlen(line) > 0) { // Ignora linhas vazias após o processamento
            strncpy(lines[line_count++], line, sizeof(line));
        }
    }

    fclose(input_file);

    // Associar rótulos à próxima instrução
    associate_labels(lines, &line_count);

    // Reordenar e escrever as seções
    reorder_sections(lines, line_count, output_file);

    fclose(output_file);

    printf("Pré-processamento concluído. Arquivo salvo como: %s\n", output_filename);
    return 0;
}
