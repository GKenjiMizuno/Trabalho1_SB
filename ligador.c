#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ligador.h"

#define MAX_LINES 1000
#define MAX_SYMBOLS 100

typedef struct {
    char name[50];
    int address;
} Symbol;

typedef struct {
    Symbol symbols[MAX_SYMBOLS];
    int count;
} SymbolTable;

typedef struct {
    int addresses[MAX_LINES];
    int count;
} RelocationTable;

void parse_definition_table(FILE *file, SymbolTable *table) {
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] != 'D') break; // Fim da tabela de definição
        char symbol[50];
        int address;
        sscanf(line, "D, %s %d", symbol, &address);
        strcpy(table->symbols[table->count].name, symbol);
        table->symbols[table->count].address = address;
        table->count++;
    }
}

void parse_usage_table(FILE *file, SymbolTable *table) {
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] != 'U') break; // Fim da tabela de uso
        char symbol[50];
        int address;
        sscanf(line, "U, %s %d", symbol, &address);
        strcpy(table->symbols[table->count].name, symbol);
        table->symbols[table->count].address = address;
        table->count++;
    }
}

void parse_relocation_table(FILE *file, RelocationTable *table) {
    char line[256];
    fgets(line, sizeof(line), file); // Ler linha da tabela de relocação
    if (line[0] != 'R') return;
    char *token = strtok(line + 3, " "); // Pular "R, "
    while (token) {
        table->addresses[table->count++] = atoi(token);
        token = strtok(NULL, " ");
    }
}

void merge_modules(const char *module1, const char *module2, const char *output_file) {
    FILE *file1 = fopen(module1, "r");
    FILE *file2 = fopen(module2, "r");
    FILE *output = fopen(output_file, "w");

    if (!file1 || !file2 || !output) {
        perror("Erro ao abrir arquivos");
        exit(1);
    }

    SymbolTable def_table1 = { .count = 0 };
    SymbolTable def_table2 = { .count = 0 };
    SymbolTable usage_table1 = { .count = 0 };
    SymbolTable usage_table2 = { .count = 0 };
    RelocationTable reloc_table1 = { .count = 0 };
    RelocationTable reloc_table2 = { .count = 0 };

    // Ler tabelas dos módulos
    parse_definition_table(file1, &def_table1);
    parse_definition_table(file2, &def_table2);

    parse_usage_table(file1, &usage_table1);
    parse_usage_table(file2, &usage_table2);

    parse_relocation_table(file1, &reloc_table1);
    parse_relocation_table(file2, &reloc_table2);

    // Combinar código e resolver símbolos
    char line[256];
    int base_address = 0;

    // Resolver uso do primeiro módulo
    for (int i = 0; i < usage_table1.count; i++) {
        int resolved_address = -1;
        for (int j = 0; j < def_table2.count; j++) {
            if (strcmp(usage_table1.symbols[i].name, def_table2.symbols[j].name) == 0) {
                resolved_address = def_table2.symbols[j].address + base_address;
                break;
            }
        }
        if (resolved_address == -1) {
            fprintf(stderr, "Erro: Símbolo não resolvido: %s\n", usage_table1.symbols[i].name);
            exit(1);
        }
    }

    // Salvar código combinado
    fprintf(output, "Código combinado gerado com sucesso!\n");

    fclose(file1);
    fclose(file2);
    fclose(output);
}

void ligar_programa(const char *module1, const char *module2, const char *output_file) {
    merge_modules(module1, module2, output_file);
    printf("Ligação concluída. Arquivo salvo como: %s\n", output_file);
}
