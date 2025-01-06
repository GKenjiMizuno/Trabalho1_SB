#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 1000
#define MAX_LABELS 100

typedef struct {
    char mnemonico[10];
    int opcode;
    int tamanho;
} OpCode;

const OpCode opcodes[] = {
    {"ADD", 1, 2},
    {"SUB", 2, 2},
    {"MUL", 3, 2},
    {"DIV", 4, 2},
    {"JMP", 5, 2},
    {"JMPN", 6, 2},
    {"JMPP", 7, 2},
    {"JMPZ", 8, 2},
    {"COPY", 9, 3},
    {"LOAD", 10, 2},
    {"STORE", 11, 2},
    {"INPUT", 12, 2},
    {"OUTPUT", 13, 2},
    {"STOP", 14, 1}
};

typedef struct {
    char label[50];
    int address;
    int defined;
} Label;

typedef struct {
    Label labels[MAX_LABELS];
    int label_count;
} SymbolTable;

// Função para encontrar o opcode
int encontrar_opcode(const char *mnemonico, int *tamanho) {
    for (int i = 0; i < sizeof(opcodes) / sizeof(OpCode); i++) {
        if (strcasecmp(opcodes[i].mnemonico, mnemonico) == 0) {
            *tamanho = opcodes[i].tamanho;
            return opcodes[i].opcode;
        }
    }
    return -1; // Instrução inválida
}

// Função para validar rótulos
int validar_rotulo(const char *label) {
    if (!isalpha(label[0]) && label[0] != '_') {
        return 0; // Rótulo inválido
    }
    for (const char *p = label + 1; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            return 0; // Rótulo contém caracteres inválidos
        }
    }
    return 1;
}

// Função para validar operandos
void validar_operandos(const char *mnemonico, int operandos) {
    int tamanho;
    int opcode = encontrar_opcode(mnemonico, &tamanho);
    if (opcode == -1) {
        fprintf(stderr, "Erro: Instrução inválida: %s\n", mnemonico);
        exit(1);
    }
    if (operandos != tamanho - 1) {
        fprintf(stderr, "Erro: Número de operandos errado para %s. Esperado: %d\n", mnemonico, tamanho - 1);
        exit(1);
    }
}

// Função para gerar o código de máquina
void gerar_codigo_maquina(const char *mnemonico, const char *operando1, const char *operando2, FILE *output_file) {
    int tamanho;
    int opcode = encontrar_opcode(mnemonico, &tamanho);

    if (opcode == -1) {
        fprintf(stderr, "Erro: Instrução inválida: %s\n", mnemonico);
        exit(1);
    }

    fprintf(output_file, "%02d ", opcode);

    if (operando1) {
        fprintf(output_file, "%s ", operando1);
    }

    if (operando2) {
        fprintf(output_file, "%s ", operando2);
    }
}

// Função principal do montador
void montar_programa(const char *input_filename, const char *output_filename) {
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

    SymbolTable sym_table = { .label_count = 0 };
    char line[256];
    int address = 0;
    int contains_begin_end = 0;

    // Primeira passagem: processar rótulos e instruções
    while (fgets(line, sizeof(line), input_file)) {
        char *token = strtok(line, " ");
        if (!token) continue;

        // Processar rótulo
        if (strchr(token, ':')) {
            char label[50];
            strncpy(label, token, strchr(token, ':') - token);
            label[strchr(token, ':') - token] = '\0';

            if (!validar_rotulo(label)) {
                fprintf(stderr, "Erro léxico no rótulo: %s\n", label);
                exit(1);
            }

            for (int i = 0; i < sym_table.label_count; i++) {
                if (strcmp(sym_table.labels[i].label, label) == 0) {
                    fprintf(stderr, "Erro: Rótulo redefinido: %s\n", label);
                    exit(1);
                }
            }

            // Adicionar rótulo à tabela de símbolos
            strcpy(sym_table.labels[sym_table.label_count].label, label);
            sym_table.labels[sym_table.label_count].address = address;
            sym_table.labels[sym_table.label_count].defined = 1;
            sym_table.label_count++;
            token = strtok(NULL, " ");
        }

        // Processar instrução ou diretiva
        if (token) {
            if (strcasecmp(token, "BEGIN") == 0 || strcasecmp(token, "END") == 0) {
                contains_begin_end = 1;
            } else if (strcasecmp(token, "SPACE") == 0) {
                fprintf(output_file, "00 ");
                address++;
            } else if (strcasecmp(token, "CONST") == 0) {
                char *value = strtok(NULL, " ");
                if (!value) {
                    fprintf(stderr, "Erro: CONST sem valor\n");
                    exit(1);
                }
                fprintf(output_file, "%s ", value);
                address++;
            } else {
                char *operando1 = strtok(NULL, " ");
                char *operando2 = strtok(NULL, " ");
                validar_operandos(token, operando2 ? 2 : (operando1 ? 1 : 0));
                gerar_codigo_maquina(token, operando1, operando2, output_file);
                address++;
            }
        }
    }

    fclose(input_file);

    // Segunda passagem: processar símbolos indefinidos
    if (contains_begin_end) {
        fprintf(output_file, "\nTabela de Símbolos:\n");
        for (int i = 0; i < sym_table.label_count; i++) {
            fprintf(output_file, "%s %d\n", sym_table.labels[i].label, sym_table.labels[i].address);
        }
    }

    fclose(output_file);
    printf("Montagem concluída. Arquivo salvo como: %s\n", output_filename);
}
