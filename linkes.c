#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 100
#define MAX_CODE_SIZE 1000
#define MAX_LINE_LENGTH 256

typedef struct {
    char name[50];
    int address;
} Symbol;

typedef struct {
    Symbol symbols[MAX_SYMBOLS];
    int count;
} DefinitionTable;

typedef struct {
    char symbol[50];
    int address;
} UseTable;

typedef struct {
    UseTable entries[MAX_SYMBOLS];
    int count;
} UseTableList;

typedef struct {
    int flags[MAX_CODE_SIZE];
    int count;
} RelocationTable;

// Função para ler uma linha e remover o caractere de nova linha
void read_line(char *line, int max_len, FILE *file) {
    if (fgets(line, max_len, file) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
    }
}

// Função para ler a tabela de definições
void read_definition_table(FILE *file, DefinitionTable *def_table) {
    char line[MAX_LINE_LENGTH];
    def_table->count = 0;

    while (1) {
        read_line(line, MAX_LINE_LENGTH, file);
        if (strncmp(line, "U,", 2) == 0 || strncmp(line, "R,", 2) == 0) {
            fseek(file, -strlen(line)-1, SEEK_CUR);
            break;
        }

        if (strncmp(line, "D,", 2) == 0) {
            char *token = strtok(line + 2, " ");
            strcpy(def_table->symbols[def_table->count].name, token);
            token = strtok(NULL, " ");
            def_table->symbols[def_table->count].address = atoi(token);
            def_table->count++;
        }
    }
}

// Função para ler a tabela de uso
void read_use_table(FILE *file, UseTableList *use_table) {
    char line[MAX_LINE_LENGTH];
    use_table->count = 0;

    while (1) {
        read_line(line, MAX_LINE_LENGTH, file);
        if (strncmp(line, "R,", 2) == 0) {
            fseek(file, -strlen(line)-1, SEEK_CUR);
            break;
        }

        if (strncmp(line, "U,", 2) == 0) {
            char *token = strtok(line + 2, " ");
            strcpy(use_table->entries[use_table->count].symbol, token);
            token = strtok(NULL, " ");
            use_table->entries[use_table->count].address = atoi(token);
            use_table->count++;
        }
    }
}

// Função para ler a tabela de relocação
void read_relocation_table(FILE *file, RelocationTable *rel_table) {
    char line[MAX_LINE_LENGTH];
    rel_table->count = 0;

    read_line(line, MAX_LINE_LENGTH, file);
    if (strncmp(line, "R,", 2) == 0) {
        char *token = strtok(line + 2, " ");
        while (token != NULL) {
            rel_table->flags[rel_table->count++] = atoi(token);
            token = strtok(NULL, " ");
        }
    }
}

// Função para ler o código objeto
void read_code(FILE *file, int *code, int *code_size) {
    char line[MAX_LINE_LENGTH];
    *code_size = 0;

    read_line(line, MAX_LINE_LENGTH, file);
    char *token = strtok(line, " ");
    while (token != NULL) {
        code[(*code_size)++] = atoi(token);
        token = strtok(NULL, " ");
    }
}

void link_programs(const char *prog1_name, const char *prog2_name) {
    FILE *prog1 = fopen(prog1_name, "r");
    FILE *prog2 = fopen(prog2_name, "r");
    FILE *output = fopen("prog1.e", "w");

    if (!prog1 || !prog2 || !output) {
        printf("Erro ao abrir arquivos\n");
        exit(1);
    }

    // Ler tabelas do primeiro programa
    DefinitionTable def_table1;
    UseTableList use_table1;
    RelocationTable rel_table1;
    int code1[MAX_CODE_SIZE];
    int code1_size;

    read_definition_table(prog1, &def_table1);
    read_use_table(prog1, &use_table1);
    read_relocation_table(prog1, &rel_table1);
    read_code(prog1, code1, &code1_size);

    // Ler tabelas do segundo programa
    DefinitionTable def_table2;
    UseTableList use_table2;
    RelocationTable rel_table2;
    int code2[MAX_CODE_SIZE];
    int code2_size;

    read_definition_table(prog2, &def_table2);
    read_use_table(prog2, &use_table2);
    read_relocation_table(prog2, &rel_table2);
    read_code(prog2, code2, &code2_size);

    // Ajustar endereços da tabela de definições do segundo programa
    for (int i = 0; i < def_table2.count; i++) {
        def_table2.symbols[i].address += code1_size;
    }

    // Ajustar código do segundo programa com base nas definições do primeiro
    for (int i = 0; i < use_table2.count; i++) {
        for (int j = 0; j < def_table1.count; j++) {
            if (strcmp(use_table2.entries[i].symbol, def_table1.symbols[j].name) == 0) {
                code2[use_table2.entries[i].address] = def_table1.symbols[j].address;
            }
        }
    }

    // Ajustar código do primeiro programa com base nas definições do segundo
    for (int i = 0; i < use_table1.count; i++) {
        for (int j = 0; j < def_table2.count; j++) {
            if (strcmp(use_table1.entries[i].symbol, def_table2.symbols[j].name) == 0) {
                code1[use_table1.entries[i].address] = def_table2.symbols[j].address;
            }
        }
    }

    // Escrever código combinado no arquivo de saída
    for (int i = 0; i < code1_size; i++) {
        fprintf(output, "%d ", code1[i]);
    }
    for (int i = 0; i < code2_size; i++) {
        fprintf(output, "%d ", code2[i]);
    }

    fclose(prog1);
    fclose(prog2);
    fclose(output);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: ./ligador <arquivo1.obj> <arquivo2.obj>\n");
        return 1;
    }

    link_programs(argv[1], argv[2]);
    printf("Ligação concluída. Arquivo de saída: prog1.e\n");

    return 0;
}