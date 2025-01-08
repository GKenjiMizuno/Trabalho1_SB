#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "montador.h"

#define MAX_LABELS 100
#define MAX_LINE_LENGTH 256

typedef struct {
    char label[50];
    int address;
    int defined;
} Label;

typedef struct {
    char label[50];
    int address_to_patch;
} PendingReference;

typedef struct {
    Label labels[MAX_LABELS];
    int label_count;
} SymbolTable;

typedef struct {
    PendingReference pending[MAX_LABELS];
    int count;
} PendingList;

const OpCode opcodes[] = {
    {"ADD", 1, 2}, {"SUB", 2, 2}, {"MULT", 3, 2}, {"DIV", 4, 2},
    {"JMP", 5, 2}, {"JMPN", 6, 2}, {"JMPP", 7, 2}, {"JMPZ", 8, 2},
    {"COPY", 9, 3}, {"LOAD", 10, 2}, {"STORE", 11, 2}, {"INPUT", 12, 2},
    {"OUTPUT", 13, 2}, {"STOP", 14, 1}
};

// Função para limpar nova linha e espaços extras
void trim_newline(char *str) {
    char *end = str + strlen(str) - 1;
    while (end >= str && (*end == '\n' || *end == '\r' || isspace((unsigned char)*end))) {
        *end = '\0';
        end--;
    }
}

// Busca o opcode para um mnemonico
int find_opcode(const char *mnemonico, int *size) {
    for (int i = 0; i < sizeof(opcodes) / sizeof(OpCode); i++) {
        if (strcasecmp(opcodes[i].mnemonico, mnemonico) == 0) {
            *size = opcodes[i].tamanho;
            return opcodes[i].opcode;
        }
    }
    return -1;
}

// Busca um endereço pelo rótulo
int find_address(SymbolTable *sym_table, PendingList *pending_list, const char *label, int current_address) {
    for (int i = 0; i < sym_table->label_count; i++) {
        if (strcmp(sym_table->labels[i].label, label) == 0) {
            return sym_table->labels[i].address;
        }
    }

    // Se não encontrado, adicionar como pendente
    strcpy(pending_list->pending[pending_list->count].label, label);
    pending_list->pending[pending_list->count].address_to_patch = current_address;
    pending_list->count++;
    return -1;
}

// Resolve as referências pendentes
void resolve_pending_references(SymbolTable *sym_table, PendingList *pending_list, FILE *output_file) {
    for (int i = 0; i < pending_list->count; i++) {
        int resolved = 0;
        for (int j = 0; j < sym_table->label_count; j++) {
            if (strcmp(pending_list->pending[i].label, sym_table->labels[j].label) == 0) {
                fseek(output_file, pending_list->pending[i].address_to_patch, SEEK_SET);
                fprintf(output_file, "%d ", sym_table->labels[j].address);
                resolved = 1;
                break;
            }
        }
        if (!resolved) {
            fprintf(stderr, "Erro: Rótulo não resolvido: %s\n", pending_list->pending[i].label);
            exit(1);
        }
    }
}

void montar_programa(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "r");
    if (!input_file) {
        perror("Erro ao abrir o arquivo de entrada");
        exit(1);
    }

    FILE *output_file = fopen(output_filename, "w+");
    if (!output_file) {
        perror("Erro ao criar o arquivo de saída");
        fclose(input_file);
        exit(1);
    }

    SymbolTable sym_table = { .label_count = 0 };
    PendingList pending_list = { .count = 0 };
    char line[MAX_LINE_LENGTH];
    int address = 0;
    int current_section = 0; // 0 = Nenhuma, 1 = TEXT, 2 = DATA
    int contains_begin_end = 0;

    // Identificar BEGIN e END
    while (fgets(line, sizeof(line), input_file)) {
        trim_newline(line);
        if (strcasecmp(line, "BEGIN") == 0) {
            contains_begin_end = 1;
        }
        if (strcasecmp(line, "END") == 0) {
            break;
        }
    }

    rewind(input_file); // Reinicia o arquivo para processamento

    while (fgets(line, sizeof(line), input_file)) {
        trim_newline(line);
        char *token = strtok(line, " ");
        if (!token) continue;

        if (strcasecmp(token, "SECTION") == 0) {
            char *section = strtok(NULL, " ");
            if (section) {
                trim_newline(section);
                if (strcasecmp(section, "TEXT") == 0) {
                    current_section = 1;
                } else if (strcasecmp(section, "DATA") == 0) {
                    current_section = 2;
                } else {
                    fprintf(stderr, "Erro: Tipo de seção inválido: %s\n", section);
                    exit(1);
                }
            } else {
                fprintf(stderr, "Erro: Falta o tipo de seção após SECTION\n");
                exit(1);
            }
            continue;
        }

        if (strchr(token, ':')) {
            char label[50];
            strncpy(label, token, strchr(token, ':') - token);
            label[strchr(token, ':') - token] = '\0';

            strcpy(sym_table.labels[sym_table.label_count].label, label);
            sym_table.labels[sym_table.label_count].address = address;
            sym_table.labels[sym_table.label_count].defined = 1;
            sym_table.label_count++;
            token = strtok(NULL, " ");
        }

        if (current_section == 1) {
            int size, opcode = find_opcode(token, &size);
            if (opcode == -1) {
                fprintf(stderr, "Erro: Instrução inválida: %s\n", token);
                exit(1);
            }
            fprintf(output_file, "%02d ", opcode);
            address++;

            for (int i = 1; i < size; i++) {
                char *operand = strtok(NULL, ", ");
                if (!operand) {
                    fprintf(stderr, "Erro: Operando ausente para %s\n", token);
                    exit(1);
                }

                int operand_address = find_address(&sym_table, &pending_list, operand, ftell(output_file));
                if (operand_address == -1) {
                    fprintf(output_file, "00 ");
                } else {
                    fprintf(output_file, "%d ", operand_address);
                }
                address++;
            }
        } else if (current_section == 2) {
            if (strcasecmp(token, "SPACE") == 0) {
                fprintf(output_file, "00 ");
                address++;
            } else if (strcasecmp(token, "CONST") == 0) {
                char *value = strtok(NULL, " ");
                fprintf(output_file, "%s ", value);
                address++;
            } else {
                fprintf(stderr, "Erro: Diretiva inválida na seção DATA: %s\n", token);
                exit(1);
            }
        }
    }

    resolve_pending_references(&sym_table, &pending_list, output_file);

    if (contains_begin_end) {
        fprintf(output_file, "\nTabela de Definição:\n");
        for (int i = 0; i < sym_table.label_count; i++) {
            if (sym_table.labels[i].defined) {
                fprintf(output_file, "D, %s %d\n", sym_table.labels[i].label, sym_table.labels[i].address);
            }
        }

        fprintf(output_file, "Tabela de Uso:\n");
        for (int i = 0; i < pending_list.count; i++) {
            fprintf(output_file, "U, %s %d\n", pending_list.pending[i].label, pending_list.pending[i].address_to_patch);
        }

        fprintf(output_file, "Relocação:\nR, ");
        for (int i = 0; i < address; i++) {
            fprintf(output_file, "1 ");
        }
        fprintf(output_file, "\n");
    }

    fclose(input_file);
    fclose(output_file);

    printf("Montagem concluída. Arquivo salvo como: %s\n", output_filename);
}
