#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYM 100
#define MAX_CODE 1024

// Estruturas principais do ligador
typedef struct {
    char symbol[50];
    int  address;
} Definition;

typedef struct {
    char symbol[50];
    int  address;  // posição no código onde o símbolo é usado
} Usage;

// Estrutura que armazena todos os dados de um arquivo .obj
typedef struct {
    Definition def_table[MAX_SYM];   // tabela de definições de símbolos
    int def_count;                   // quantidade de definições

    Usage use_table[MAX_SYM];        // tabela de usos (referências externas)
    int use_count;                   // quantidade de usos

    int reloc[MAX_CODE];             // bits de relocação para cada palavra do código
    int code_size;                   // tamanho do código

    int code[MAX_CODE];              // código de máquina
} ObjModule;


// Declarações das funções principais
void parse_obj_file(
    const char *filename,
    ObjModule *module
);

void link_two_modules(
    ObjModule *m1,
    ObjModule *m2,
    const char *output_filename
);

int find_symbol_in_def_table(Definition *defs, int def_count, const char *sym);
void error_exit(const char *msg);

// Função principal
int main(int argc, char *argv[])
{
    if(argc < 2) {
        fprintf(stderr, "Uso: %s mod1.obj [mod2.obj]\n", argv[0]);
        exit(1);
    }

    // Processa primeiro módulo
    ObjModule module1;
    memset(&module1, 0, sizeof(ObjModule));
    parse_obj_file(argv[1], &module1);

    // Se houver segundo módulo, processa ele também
    ObjModule module2;
    memset(&module2, 0, sizeof(ObjModule));

    int has_second = (argc >= 3);
    if(has_second) {
        parse_obj_file(argv[2], &module2);
    }

    // Cria nome do arquivo de saída substituindo extensão .obj por .e
    char output_file[256];
    strncpy(output_file, argv[1], sizeof(output_file));
    char *dot = strrchr(output_file, '.');
    if(dot) {
        strcpy(dot, ".e");
    } else {
        strcat(output_file, ".e");
    }

    // Realiza a ligação dos módulos
    if(!has_second) {
        // Caso especial: apenas um módulo
        link_two_modules(&module1, NULL, output_file);
    } else {
        link_two_modules(&module1, &module2, output_file);
    }

    printf("Ligação concluída. Gerado arquivo %s\n", output_file);
    return 0;
}

// Função que processa um arquivo .obj e preenche a estrutura ObjModule
// Formato esperado do arquivo:
// D, SIMBOLO ENDERECO  (definições)
// U, SIMBOLO ENDERECO  (usos)
// R, 0 1 0 1 0...     (bits de relocação)
// 10 9 1 0 11...      (código de máquina)
void parse_obj_file(const char *filename, ObjModule *module)
{
    FILE *fp = fopen(filename, "r");
    if(!fp) {
        fprintf(stderr, "Erro ao abrir arquivo %s\n", filename);
        exit(1);
    }

    char line[512];
    while(fgets(line, sizeof(line), fp)) {
        // Remove quebra de linha
        char *nl = strchr(line, '\n');
        if(nl) *nl = '\0';

        if(strlen(line) == 0) continue;

        // Processa linha de definição (D,)
        if(strncmp(line, "D,", 2) == 0) {
            char dummy[4], sym[50];
            int addr;
            if(sscanf(line, "%3[^,], %s %d", dummy, sym, &addr) == 3) {
                if(module->def_count >= MAX_SYM) {
                    error_exit("Muitas definições no arquivo.");
                }
                strcpy(module->def_table[module->def_count].symbol, sym);
                module->def_table[module->def_count].address = addr;
                module->def_count++;
            }
        }
        // Processa linha de uso (U,)
        else if(strncmp(line, "U,", 2) == 0) {
            char dummy[4], sym[50];
            int addr;
            if(sscanf(line, "%3[^,], %s %d", dummy, sym, &addr) == 3) {
                if(module->use_count >= MAX_SYM) {
                    error_exit("Muitas referências externas no arquivo.");
                }
                strcpy(module->use_table[module->use_count].symbol, sym);
                module->use_table[module->use_count].address = addr;
                module->use_count++;
            }
        }
        // Processa linha de bits de relocação (R,)
        else if(strncmp(line, "R,", 2) == 0) {
            char *p = strchr(line, ',');
            if(!p) continue;
            p++;
            char *tok = strtok(p, " \t");
            int idx = 0;
            while(tok) {
                module->reloc[idx++] = atoi(tok);
                tok = strtok(NULL, " \t");
            }
            module->code_size = idx;
        }
        // Processa linha de código de máquina
        else {
            int count = 0;
            char *tok = strtok(line, " \t");
            while(tok) {
                module->code[count++] = atoi(tok);
                tok = strtok(NULL, " \t");
            }
            if(count != module->code_size) {
                fprintf(stderr,
                    "Aviso: número de palavras de código (%d) difere de relocation size (%d) em %s.\n",
                    count, module->code_size, filename
                );
            }
        }
    }

    fclose(fp);
}

// Função principal de ligação que combina dois módulos em um executável
// Se m2 for NULL, apenas processa m1
// Passos principais:
// 1. Combina o código dos dois módulos
// 2. Ajusta endereços do segundo módulo
// 3. Resolve referências entre módulos
// 4. Gera arquivo executável final
void link_two_modules(ObjModule *m1, ObjModule *m2, const char *output_filename)
{
    int final_code[MAX_CODE];
    int final_reloc[MAX_CODE];
    memset(final_code, 0, sizeof(final_code));
    memset(final_reloc, 0, sizeof(final_reloc));

    // Copia código do primeiro módulo
    int offset = 0;
    for(int i=0; i < m1->code_size; i++) {
        final_code[i] = m1->code[i];
        final_reloc[i] = m1->reloc[i];
    }
    offset += m1->code_size;

    // Se existe segundo módulo, copia seu código
    int offset_module2 = offset;
    int total_size = m1->code_size;
    if(m2) {
        for(int i=0; i < m2->code_size; i++) {
            final_code[offset + i] = m2->code[i];
            final_reloc[offset + i] = m2->reloc[i];
        }
        offset += m2->code_size;
        total_size += m2->code_size;
    }

    // Cria tabela combinada de definições
    Definition combined_defs[MAX_SYM * 2];
    int combined_count = 0;
    
    // Copia definições do primeiro módulo
    for(int i=0; i < m1->def_count; i++) {
        combined_defs[combined_count++] = m1->def_table[i];
    }
    
    // Copia definições do segundo módulo ajustando endereços
    if(m2) {
        for(int i=0; i < m2->def_count; i++) {
            Definition def = m2->def_table[i];
            def.address += offset_module2;
            combined_defs[combined_count++] = def;
        }
    }

    // Resolve referências do primeiro módulo
    for(int i=0; i < m1->use_count; i++) {
        Usage *u = &m1->use_table[i];
        int idx = find_symbol_in_def_table(combined_defs, combined_count, u->symbol);
        if(idx < 0) {
            fprintf(stderr, "ERRO: Símbolo '%s' não definido em nenhum módulo.\n", u->symbol);
            exit(1);
        }
        int real_address = combined_defs[idx].address;
        final_code[u->address] = real_address;
        final_reloc[u->address] = 0;
    }

    // Resolve referências do segundo módulo
    if(m2) {
        for(int i=0; i < m2->use_count; i++) {
            Usage *u = &m2->use_table[i];
            int usage_address_in_final = offset_module2 + u->address;

            int idx = find_symbol_in_def_table(combined_defs, combined_count, u->symbol);
            if(idx < 0) {
                fprintf(stderr, "ERRO: Símbolo '%s' não definido em nenhum módulo.\n", u->symbol);
                exit(1);
            }
            int real_address = combined_defs[idx].address;
            final_code[usage_address_in_final] = real_address;
            final_reloc[usage_address_in_final] = 0;
        }
    }

    // Gera arquivo executável final
    FILE *out = fopen(output_filename, "w");
    if(!out) {
        perror("Erro criando arquivo de saída");
        exit(1);
    }

    for(int i=0; i < total_size; i++) {
        fprintf(out, "%d ", final_code[i]);
    }
    fprintf(out, "\n");

    fclose(out);
}

// Busca um símbolo na tabela de definições
// Retorna o índice se encontrar ou -1 caso contrário
int find_symbol_in_def_table(Definition *defs, int def_count, const char *sym)
{
    for(int i=0; i < def_count; i++) {
        if(strcasecmp(defs[i].symbol, sym) == 0) {
            return i;
        }
    }
    return -1;
}

// Função auxiliar para exibir erro e encerrar o programa
void error_exit(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}