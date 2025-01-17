#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Constantes para limites do programa
#define MAX_LABELS     100        // Número máximo de rótulos permitidos
#define MAX_LINE_LENGTH 256       // Tamanho máximo de uma linha do código fonte
#define MAX_CODE_SIZE   1024      // Tamanho máximo do código objeto gerado

#include <stddef.h>

// Função auxiliar para busca de strings sem diferenciar maiúsculas/minúsculas
char *strcasestr(const char *haystack, const char *needle);

// Implementação da função strcasestr para sistemas que não a possuem nativamente
char *strcasestr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;

    size_t needle_len = strlen(needle);
    if (needle_len == 0) return (char *)haystack;

    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
    }
    return NULL;
}

// Tabela de instruções do assembly inventado
// Cada instrução tem um mnemônico, código de operação e tamanho em palavras
typedef struct {
    char mnemonico[10];  // Nome da instrução (ex: ADD, SUB)
    int  opcode;         // Código numérico da operação
    int  tamanho;        // Quantidade de palavras que a instrução ocupa
} OpCode;

// Lista de todas as instruções suportadas com seus respectivos códigos e tamanhos
const OpCode opcodes[] = {
    {"ADD", 1, 2}, {"SUB", 2, 2}, {"MULT", 3, 2}, {"DIV", 4, 2},
    {"JMP", 5, 2}, {"JMPN", 6, 2}, {"JMPP", 7, 2}, {"JMPZ", 8, 2},
    {"COPY", 9, 3}, {"LOAD", 10,2}, {"STORE",11,2}, {"INPUT",12,2},
    {"OUTPUT",13,2}, {"STOP",14,1}
};

// Estruturas para a tabela de símbolos
typedef struct {
    char name[50];     // Nome do rótulo
    int  address;      // Endereço onde o rótulo foi definido
    int  is_defined;   // Indica se o rótulo já foi definido no módulo
    int  is_extern;    // Indica se o rótulo é externo (definido em outro módulo)
    int  is_public;    // Indica se o rótulo é público (pode ser usado por outros módulos)
} Label;

// Estrutura para referências ainda não resolvidas
typedef struct {
    char label[50];               // Nome do rótulo
    int  instruction_address;     // Endereço da instrução que usa o rótulo
} PendingReference;

// Tabela de símbolos completa
typedef struct {
    Label labels[MAX_LABELS];           // Lista de rótulos
    int   label_count;                  // Quantidade de rótulos na tabela

    PendingReference pendings[MAX_LABELS];  // Lista de referências pendentes
    int             pending_count;          // Quantidade de referências pendentes
} SymbolTable;

// Declarações antecipadas das funções principais
int  find_opcode(const char *mnemonico, int *size);
int  is_valid_label(const char *lbl);
void trim_newline(char *str);

void add_label(SymbolTable *sym, const char* name, int address,
               int is_extern, int is_public, int is_defined);
void add_pending(SymbolTable *sym, const char *label, int instr_address);
int  get_label_address(SymbolTable *sym, const char* label);
void fix_pending(SymbolTable *sym, int *code, int code_size, int *reloc);

void print_module_output(SymbolTable *sym, int *code, int code_size, int *reloc, FILE *out);
void print_flat_output(int *code, int code_size, FILE *out);

// Função principal do montador
void montar_programa(const char *input_filename, const char *output_filename);

// Busca uma instrução na tabela de opcodes e retorna seu código
// Retorna -1 se não encontrar e atualiza o tamanho da instrução
int find_opcode(const char *mnemonico, int *size)
{
    int nops = (int)(sizeof(opcodes) / sizeof(opcodes[0]));
    for(int i = 0; i < nops; i++) {
        if(strcasecmp(opcodes[i].mnemonico, mnemonico) == 0) {
            *size = opcodes[i].tamanho;
            return opcodes[i].opcode;
        }
    }
    return -1;
}

// Verifica se um rótulo é válido sintaticamente:
// - Deve começar com letra
// - Pode conter letras, números e underscore
int is_valid_label(const char *lbl)
{
    if(!isalpha((unsigned char)lbl[0])) return 0;
    for(int i = 1; lbl[i] != '\0'; i++){
        if(!isalnum((unsigned char)lbl[i]) && lbl[i] != '_') {
            return 0;
        }
    }
    return 1;
}

// Remove espaços e quebras de linha do final da string
void trim_newline(char *str)
{
    char *end = str + strlen(str) - 1;
    while(end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

// Adiciona ou atualiza um rótulo na tabela de símbolos
// Gera erro se tentar redefinir um rótulo já definido
void add_label(SymbolTable *sym, const char* name, int address,
               int is_extern, int is_public, int is_defined)
{
    if (!is_valid_label(name)) {
        fprintf(stderr, "ERRO: Rótulo inválido '%s'.\n", name);
        exit(1);
    }

    // Procura se o rótulo já existe
    for(int i = 0; i < sym->label_count; i++) {
        if(strcasecmp(sym->labels[i].name, name) == 0) {
            if(is_defined && sym->labels[i].is_defined) {
                fprintf(stderr, "ERRO: Rótulo '%s' redefinido.\n", name);
                exit(1);
            }
            // Atualiza as informações do rótulo existente
            if(is_defined) {
                sym->labels[i].address = address;
                sym->labels[i].is_defined = 1;
            }
            if(is_public)
                sym->labels[i].is_public = 1;
            if(is_extern) {
                sym->labels[i].address = 0;
                sym->labels[i].is_defined = 0;
                sym->labels[i].is_extern = 1;
            }
            return;
        }
    }

    // Se não encontrou, adiciona novo rótulo
    if(sym->label_count >= MAX_LABELS) {
        fprintf(stderr, "ERRO: Excedido número máximo de rótulos.\n");
        exit(1);
    }

    strcpy(sym->labels[sym->label_count].name, name);
    sym->labels[sym->label_count].address = address;
    sym->labels[sym->label_count].is_extern = is_extern;
    sym->labels[sym->label_count].is_public = is_public;
    sym->labels[sym->label_count].is_defined = is_defined;
    sym->label_count++;
}

// Adiciona uma referência pendente para ser resolvida depois
void add_pending(SymbolTable *sym, const char *label, int instr_address)
{
    if(sym->pending_count >= MAX_LABELS) {
        fprintf(stderr, "ERRO: Excedido número máximo de pendências.\n");
        exit(1);
    }
    strcpy(sym->pendings[sym->pending_count].label, label);
    sym->pendings[sym->pending_count].instruction_address = instr_address;
    sym->pending_count++;
}

// Busca o endereço de um rótulo na tabela de símbolos
// Retorna -1 se não encontrar
int get_label_address(SymbolTable *sym, const char* label)
{
    for(int i = 0; i < sym->label_count; i++) {
        if(strcasecmp(sym->labels[i].name, label) == 0) {
            return sym->labels[i].address;
        }
    }
    return -1;
}

// Resolve todas as referências pendentes no código
// Preenche o vetor de relocação para linking
void fix_pending(SymbolTable *sym, int *code, int code_size, int *reloc)
{
    for(int i = 0; i < sym->pending_count; i++) {
        PendingReference *p = &sym->pendings[i];
        int addr = get_label_address(sym, p->label);
        if(addr < 0) {
            fprintf(stderr,"ERRO: Rótulo '%s' não definido.\n", p->label);
            exit(1);
        }

        // Verifica se o rótulo é externo
        int is_ext = 0;
        for(int j = 0; j < sym->label_count; j++){
            if(strcasecmp(sym->labels[j].name, p->label) == 0) {
                is_ext = sym->labels[j].is_extern;
                break;
            }
        }

        // Referências externas têm endereço 0 e bit de relocação 1
        if(is_ext) {
            code[p->instruction_address] = 0;
            reloc[p->instruction_address] = 1;
        } else {
            code[p->instruction_address] = addr;
            reloc[p->instruction_address] = 1;
        }
    }
}

// Gera saída no formato de módulo com tabelas de definição e uso
void print_module_output(SymbolTable *sym, int *code, int code_size, int *reloc, FILE *out)
{
    // Tabela de definições (rótulos públicos)
    for(int i = 0; i < sym->label_count; i++) {
        if(sym->labels[i].is_public && sym->labels[i].is_defined) {
            fprintf(out, "D, %s %d\n", sym->labels[i].name, sym->labels[i].address);
        }
    }

    // Tabela de uso (referências a símbolos externos)
    for(int i = 0; i < sym->pending_count; i++){
        for(int j = 0; j < sym->label_count; j++){
            if(strcasecmp(sym->pendings[i].label, sym->labels[j].name) == 0){
                if(sym->labels[j].is_extern){
                    fprintf(out, "U, %s %d\n",
                            sym->pendings[i].label,
                            sym->pendings[i].instruction_address);
                }
            }
        }
    }

    // Bits de relocação
    fprintf(out, "R, ");
    for(int i = 0; i < code_size; i++){
        fprintf(out, "%d ", reloc[i]);
    }
    fprintf(out, "\n");

    // Código objeto final
    for(int i = 0; i < code_size; i++){
        fprintf(out, "%d ", code[i]);
    }
    fprintf(out, "\n");
}

// Gera saída simples apenas com o código objeto
void print_flat_output(int *code, int code_size, FILE *out)
{
    for(int i = 0; i < code_size; i++){
        fprintf(out, "%d ", code[i]);
    }
    fprintf(out, "\n");
}

// Função Principal do Montador
void montar_programa(const char *input_filename, const char *output_filename)
{
    FILE *fp = fopen(input_filename, "r");
    if(!fp){
        perror("Erro ao abrir arquivo de entrada");
        exit(1);
    }

    // Inicializa vetores do código objeto e bits de relocação
    int code[MAX_CODE_SIZE];
    int reloc[MAX_CODE_SIZE];
    for(int i = 0; i < MAX_CODE_SIZE; i++) {
        code[i]  = 0;
        reloc[i] = 0;
    }

    // Inicializa tabela de símbolos vazia
    SymbolTable sym;
    sym.label_count   = 0;
    sym.pending_count = 0;

    int code_size       = 0;  // Contador de palavras no código objeto
    int current_section = 0;  // Seção atual (1=TEXT, 2=DATA) 
    int has_begin_end   = 0;  // Flag para detectar diretivas BEGIN/END

    char line[MAX_LINE_LENGTH];

    // Passagem prévia: verifica se é um módulo (tem BEGIN/END)
    while(fgets(line, sizeof(line), fp)){
        trim_newline(line);
        if(strcasestr(line, "BEGIN")) {
            has_begin_end = 1;
        }
    }
    rewind(fp);

    // Primeira Passagem: 
    // - Coleta todos os rótulos e seus endereços
    // - Calcula tamanho total do código
    code_size = 0;
    while(fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if(strlen(line) == 0) continue;

        char *tk = strtok(line, " \t");
        if(!tk) continue;

        // Processa rótulos (terminados em :)
        if(strchr(tk, ':')) {
            char lbl[50];
            char *c = strchr(tk, ':');
            int len = (int)(c - tk);
            if(len <= 0 || len >= (int)sizeof(lbl)) {
                fprintf(stderr, "ERRO: Sintaxe de rótulo inválida '%s'.\n", tk);
                exit(1);
            }
            strncpy(lbl, tk, len);
            lbl[len] = '\0';

            // Verifica se é rótulo EXTERN
            char *lookahead = strtok(NULL, " \t");
            if(lookahead && strcasecmp(lookahead, "EXTERN") == 0) {
                add_label(&sym, lbl, 0, 1, 0, 0);
                continue;
            }
            else {
                add_label(&sym, lbl, code_size, 0, 0, 1);
                tk = lookahead;
                if(!tk) continue;
            }
        }

        if(!tk) continue;

        // Processa diretivas SECTION
        if(strcasecmp(tk, "SECTION") == 0) {
            char *secname = strtok(NULL, " \t");
            if(secname) {
                if(strcasecmp(secname, "TEXT") == 0) {
                    current_section = 1;
                } else if(strcasecmp(secname, "DATA") == 0) {
                    current_section = 2;
                }
            }
            continue;
        }

        // Processa diretivas PUBLIC e EXTERN
        if(strcasecmp(tk, "PUBLIC") == 0){
            char *lbl = strtok(NULL," \t");
            if(!lbl) {
                fprintf(stderr, "ERRO: Faltou nome após PUBLIC.\n");
                exit(1);
            }
            add_label(&sym, lbl, 0, 0, 1, 0);
            continue;
        }
        if(strcasecmp(tk, "EXTERN") == 0){
            char *lbl = strtok(NULL, " \t");
            if(lbl){
                add_label(&sym, lbl, 0, 1, 0, 0);
            }
            continue;
        }

        // Ignora diretivas BEGIN/END nesta passagem
        if(strcasecmp(tk, "BEGIN") == 0){
            continue;
        }
        if(strcasecmp(tk, "END") == 0){
            continue;
        }

        // Na seção TEXT: soma tamanho das instruções
        if(current_section == 1) {
            int size;
            int op = find_opcode(tk, &size);
            if(op >= 0) {
                code_size += size;
            }
        }
        // Na seção DATA: soma tamanho das diretivas
        else if(current_section == 2) {
            if(strcasecmp(tk, "SPACE") == 0){
                code_size += 1;
            }
            else if(strcasecmp(tk, "CONST") == 0){
                code_size += 1;
            }
        }
    }
    rewind(fp);

    // Segunda Passagem:
    // - Gera código objeto
    // - Resolve referências a rótulos
    // - Marca bits de relocação
    code_size = 0;
    current_section = 0;
    while(fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if(strlen(line) == 0) continue;

        char *tk = strtok(line, " \t");
        if(!tk) continue;

        // Pula rótulos (já processados)
        if(strchr(tk, ':')) {
            tk = strtok(NULL, " \t");
            if(!tk) continue;
        }

        // Atualiza seção atual
        if(strcasecmp(tk, "SECTION") == 0) {
            char *sec = strtok(NULL, " \t");
            if(sec){
                if(strcasecmp(sec, "TEXT") == 0) {
                    current_section = 1;
                } else if(strcasecmp(sec, "DATA") == 0) {
                    current_section = 2;
                }
            }
            continue;
        }

        // Pula diretivas de ligação
        if(strcasecmp(tk, "PUBLIC") == 0){
            strtok(NULL," \t");
            continue;
        }
        if(strcasecmp(tk, "EXTERN") == 0){
            strtok(NULL," \t");
            continue;
        }

        // Pula diretivas de módulo
        if(strcasecmp(tk, "BEGIN") == 0){
            strtok(NULL," \t");
            continue;
        }
        if(strcasecmp(tk, "END") == 0){
            continue;
        }

        // Processa instruções na seção TEXT
        if(current_section == 1) {
            int size = 0;
            int op = find_opcode(tk, &size);
            if(op >= 0) {
                // Gera código do opcode
                code[code_size] = op;
                reloc[code_size] = 0;
                code_size++;

                // Trata operandos
                if(strcasecmp(tk, "COPY") == 0 && size == 3) {
                    // COPY tem sintaxe especial: COPY X,Y
                    char *operand = strtok(NULL, " \t");
                    if(!operand) {
                        fprintf(stderr, "ERRO: Operandos faltando para COPY.\n");
                        exit(1);
                    }
                    char *first  = strtok(operand, ",");
                    char *second = strtok(NULL, ",");
                    if(!first || !second) {
                        fprintf(stderr, "ERRO: COPY requer 'SRC,DST'.\n");
                        exit(1);
                    }
                    code[code_size] = 0;
                    add_pending(&sym, first, code_size);
                    code_size++;

                    code[code_size] = 0;
                    add_pending(&sym, second, code_size);
                    code_size++;
                }
                else {
                    // Demais instruções: operandos separados por espaço
                    for(int i = 1; i < size; i++) {
                        char *operand = strtok(NULL, " ,\t");
                        if(!operand) {
                            fprintf(stderr, "ERRO: Faltam operandos para '%s'.\n", tk);
                            exit(1);
                        }
                        code[code_size] = 0;
                        add_pending(&sym, operand, code_size);
                        code_size++;
                    }
                }
            }
            else {
                fprintf(stderr, "ERRO: Instrução desconhecida '%s'.\n", tk);
                exit(1);
            }
        }
        // Processa diretivas na seção DATA
        else if(current_section == 2) {
            if(strcasecmp(tk, "SPACE") == 0) {
                code[code_size] = 0;
                reloc[code_size] = 0;
                code_size++;
            }
            else if(strcasecmp(tk, "CONST") == 0) {
                char *val = strtok(NULL, " \t");
                if(!val) {
                    fprintf(stderr, "ERRO: Falta valor em CONST.\n");
                    exit(1);
                }
                int number;
                if(strncasecmp(val, "0x", 2) == 0) {
                    number = (int)strtol(val, NULL, 16);
                } else {
                    number = atoi(val);
                }
                code[code_size] = number;
                reloc[code_size] = 0;
                code_size++;
            }
            else {
                fprintf(stderr, "ERRO: Diretiva desconhecida '%s'.\n", tk);
                exit(1);
            }
        }
    }
    fclose(fp);

    // Resolve referências pendentes
    fix_pending(&sym, code, code_size, reloc);

    // Gera arquivo de saída
    FILE *out = fopen(output_filename, "w");
    if(!out) {
        perror("Erro ao criar arquivo de saída");
        exit(1);
    }

    // Escolhe formato de saída baseado na presença de BEGIN/END
    if(has_begin_end){
        print_module_output(&sym, code, code_size, reloc, out);
    } else {
        print_flat_output(code, code_size, out);
    }

    fclose(out);
}
