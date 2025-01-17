#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 1000
#define MAX_MACROS 100
#define MAX_MACRO_LINES 50

// Estrutura para armazenar informações sobre macros
typedef struct {
    char name[50];               // Nome da macro
    char lines[MAX_MACRO_LINES][256]; // Corpo da macro (código associado)
    int line_count;              // Número de linhas dentro da macro
} Macro;

Macro macros[MAX_MACROS]; // Lista de macros definidas
int macro_count = 0;      // Contador de macros registradas

// Função para processar uma linha removendo espaços extras, comentários e convertendo para maiúsculas
void preprocess_line(char *line) {
    char *comment = strchr(line, ';'); // Identifica início de um comentário
    if (comment) {
        *comment = '\0'; // Remove comentário
    }

    // Remove espaços em branco no início da linha
    char *start = line;
    while (isspace((unsigned char)*start)) start++;

    // Remove espaços em branco no final da linha
    char *end = line + strlen(line) - 1;
    while (end > start && isspace((unsigned char)*end)) *end-- = '\0';

    // Move a linha processada para o início do buffer
    memmove(line, start, strlen(start) + 1);

    // Converte todos os caracteres para maiúsculas
    for (char *p = line; *p; p++) {
        *p = toupper((unsigned char)*p);
    }
}

// Verifica se a linha contém diretivas EQU ou IF, que devem ser ignoradas
int is_equ_or_if(const char *line) {
    if (strstr(line, "EQU") || strstr(line, "IF")) {
        return 1; // Se for uma dessas diretivas, retorna verdadeiro
    }
    return 0;
}

// Busca uma macro pelo nome e retorna seu índice
int find_macro(const char *name) {
    for (int i = 0; i < macro_count; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            return i; // Retorna o índice da macro encontrada
        }
    }
    return -1; // Retorna -1 se a macro não for encontrada
}

// Função para processar um arquivo de entrada e gerar um arquivo pré-processado ou montado
void preprocess_file(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "r");
    if (!input_file) {
        perror("Erro ao abrir o arquivo de entrada"); // Exibe mensagem de erro se o arquivo não for encontrado
        exit(1);
    }

    FILE *output_file = fopen(output_filename, "w");
    if (!output_file) {
        perror("Erro ao criar o arquivo de saída"); // Exibe mensagem de erro ao tentar criar o arquivo de saída
        fclose(input_file);
        exit(1);
    }

    char line[256];
    int inside_macro = 0; // Flag para indicar se estamos dentro de uma definição de macro
    Macro current_macro;  // Variável para armazenar a macro que está sendo definida

    // Lê o arquivo linha por linha
    while (fgets(line, sizeof(line), input_file)) {
        preprocess_line(line); // Remove espaços, comentários e converte para maiúsculas

        if (strlen(line) == 0) {
            continue; // Ignora linhas vazias
        }

        if (is_equ_or_if(line)) {
            continue; // Ignora diretivas EQU e IF
        }

        // Identifica início de uma macro
        if (strncmp(line, "MACRO", 5) == 0) {
            inside_macro = 1;
            current_macro.line_count = 0;
            char *macro_name = strtok(line + 5, " "); // Obtém o nome da macro
            if (!macro_name) {
                fprintf(stderr, "Erro: Nome de macro ausente após 'MACRO'\n");
                exit(1);
            }
            strcpy(current_macro.name, macro_name); // Armazena o nome da macro
            continue;
        }

        // Identifica final de uma macro
        if (inside_macro && strncmp(line, "ENDMACRO", 8) == 0) {
            inside_macro = 0;
            if (macro_count >= MAX_MACROS) {
                fprintf(stderr, "Erro: Número máximo de macros excedido\n");
                exit(1);
            }
            macros[macro_count++] = current_macro; // Armazena a macro na lista
            continue;
        }

        // Se estiver dentro de uma macro, adiciona a linha ao corpo da macro
        if (inside_macro) {
            if (current_macro.line_count >= MAX_MACRO_LINES) {
                fprintf(stderr, "Erro: Número máximo de linhas na macro excedido\n");
                exit(1);
            }
            strcpy(current_macro.lines[current_macro.line_count++], line);
            continue;
        }

        // Verifica se a linha corresponde a uma macro já definida
        int macro_index = find_macro(line);
        if (macro_index != -1) {
            for (int i = 0; i < macros[macro_index].line_count; i++) {
                fprintf(output_file, "%s\n", macros[macro_index].lines[i]); // Expande a macro no arquivo de saída
            }
            continue;
        }

        // Se não for macro, escreve a linha processada no arquivo de saída
        fprintf(output_file, "%s\n", line);
    }

    // Fecha os arquivos após o processamento
    fclose(input_file);
    fclose(output_file);
}
