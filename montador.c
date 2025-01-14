#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LABELS     100
#define MAX_LINE_LENGTH 256
#define MAX_CODE_SIZE   1024 // Adjust if needed

/*********************************
 *         OPCODE TABLE
 *********************************/
typedef struct {
    char mnemonico[10];
    int  opcode;
    int  tamanho;  // how many words (including the opcode)
} OpCode;

const OpCode opcodes[] = {
    {"ADD", 1, 2}, {"SUB", 2, 2}, {"MULT", 3, 2}, {"DIV", 4, 2},
    {"JMP", 5, 2}, {"JMPN", 6, 2}, {"JMPP", 7, 2}, {"JMPZ", 8, 2},
    {"COPY", 9, 3}, {"LOAD", 10,2}, {"STORE",11,2}, {"INPUT",12,2},
    {"OUTPUT",13,2}, {"STOP",14,1}
};

/*********************************
 *         SYMBOL TABLE
 *********************************/
typedef struct {
    char name[50];
    int  address;    // address in code
    int  is_defined; // 1 if label is defined in this module
    int  is_extern;  // 1 if label is EXTERN
    int  is_public;  // 1 if label is PUBLIC
} Label;

typedef struct {
    char label[50];
    int  instruction_address; // which code[] index references this label
} PendingReference;

typedef struct {
    Label           labels[MAX_LABELS];
    int             label_count;

    PendingReference pendings[MAX_LABELS];
    int             pending_count;
} SymbolTable;

/*********************************
 *     FORWARD DECLARATIONS
 *********************************/
int  find_opcode(const char *mnemonico, int *size);
int  is_valid_label(const char *lbl);
void trim_newline(char *str);

void add_label(SymbolTable *sym, const char* name, int address,
               int is_extern, int is_public, int is_defined);
void add_pending(SymbolTable *sym, const char *label, int instr_address);
int  get_label_address(SymbolTable *sym, const char* label);
void fix_pending(SymbolTable *sym, int *code, int code_size, int *reloc);
void print_module_output(SymbolTable *sym, int *code, int code_size, int *reloc);
void print_flat_output(int *code, int code_size);

/* Assembler core function (two-pass) */
void montar_programa(const char *input_filename, const char *output_filename);

// Returns -1 if not found; otherwise returns opcode in [1..14], plus sets *size
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

// Checks if a label is lexically valid:
//  1) Must start with letter (A-Z or a-z)
//  2) Then can have letters, digits, or underscore
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

// Removes trailing newline/spaces from a string
void trim_newline(char *str)
{
    char *end = str + strlen(str) - 1;
    while(end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

// Add or update label in the symbol table
// Raises error on label redefinition.
void add_label(SymbolTable *sym, const char* name, int address,
               int is_extern, int is_public, int is_defined)
{
    // Lexical check
    if (!is_valid_label(name)) {
        fprintf(stderr, "ERRO: Rótulo inválido '%s'.\n", name);
        exit(1);
    }

    // Check if it already exists
    for(int i = 0; i < sym->label_count; i++) {
        if(strcasecmp(sym->labels[i].name, name) == 0) {
            // If we're defining it again, check for redefinition
            if(is_defined && sym->labels[i].is_defined) {
                fprintf(stderr, "ERRO: Rótulo '%s' redefinido.\n", name);
                exit(1);
            }
            // Merge or update fields
            if(is_defined) {
                sym->labels[i].address    = address;
                sym->labels[i].is_defined = 1;
            }
            if(is_public)
                sym->labels[i].is_public = 1;
            if(is_extern) {
                // If it was previously defined, revert
                sym->labels[i].address    = 0;
                sym->labels[i].is_defined = 0;
                sym->labels[i].is_extern  = 1;
            }
            return; // done
        }
    }

    // If not found, add new
    if(sym->label_count >= MAX_LABELS) {
        fprintf(stderr, "ERRO: Excedido número máximo de rótulos.\n");
        exit(1);
    }

    strcpy(sym->labels[sym->label_count].name, name);
    sym->labels[sym->label_count].address    = address;
    sym->labels[sym->label_count].is_extern  = is_extern;
    sym->labels[sym->label_count].is_public  = is_public;
    sym->labels[sym->label_count].is_defined = is_defined;
    sym->label_count++;
}

// Add a reference that must be resolved later (pass2 fix-up)
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

// Return the address of label or -1 if not found
int get_label_address(SymbolTable *sym, const char* label)
{
    for(int i = 0; i < sym->label_count; i++) {
        if(strcasecmp(sym->labels[i].name, label) == 0) {
            return sym->labels[i].address;
        }
    }
    return -1; // not found
}

// Second pass fix for code references
void fix_pending(SymbolTable *sym, int *code, int code_size, int *reloc)
{
    for(int i = 0; i < sym->pending_count; i++) {
        PendingReference *p = &sym->pendings[i];
        int addr = get_label_address(sym, p->label);
        if(addr < 0) {
            fprintf(stderr,"ERRO: Rótulo '%s' não definido.\n", p->label);
            exit(1);
        }

        // Determine if label is extern
        int is_ext = 0;
        for(int j = 0; j < sym->label_count; j++){
            if(strcasecmp(sym->labels[j].name, p->label) == 0) {
                is_ext = sym->labels[j].is_extern;
                break;
            }
        }

        // According to your example, even extern references have reloc=1
        // and code is 0
        if(is_ext) {
            code[p->instruction_address]   = 0;
            reloc[p->instruction_address]  = 1; // forcing to 1 to match user’s example
        } else {
            code[p->instruction_address]   = addr;
            reloc[p->instruction_address]  = 1;
        }
    }
}

// Print module format (BEGIN/END present)
void print_module_output(SymbolTable *sym, int *code, int code_size, int *reloc)
{
    // 1) Definition table
    //    D, label address
    for(int i = 0; i < sym->label_count; i++) {
        if(sym->labels[i].is_public && sym->labels[i].is_defined) {
            printf("D, %s %d\n", sym->labels[i].name, sym->labels[i].address);
        }
    }

    // 2) Usage table
    //    U, label address_of_code_usage
    for(int i = 0; i < sym->pending_count; i++){
        for(int j = 0; j < sym->label_count; j++){
            if(strcasecmp(sym->pendings[i].label, sym->labels[j].name) == 0){
                if(sym->labels[j].is_extern){
                    printf("U, %s %d\n",
                           sym->pendings[i].label,
                           sym->pendings[i].instruction_address);
                }
            }
        }
    }

    // 3) Relocation bits
    printf("R, ");
    for(int i = 0; i < code_size; i++){
        printf("%d ", reloc[i]);
    }
    printf("\n");

    // 4) Final code
    for(int i = 0; i < code_size; i++){
        printf("%d ", code[i]);
    }
    printf("\n");
}

// Print flat format (no BEGIN/END)
void print_flat_output(int *code, int code_size)
{
    for(int i = 0; i < code_size; i++){
        printf("%d ", code[i]);
    }
    printf("\n");
}

/*********************************
 *      ASSEMBLER FUNCTION
 *********************************/
void montar_programa(const char *input_filename, const char *output_filename)
{
    FILE *fp = fopen(input_filename, "r");
    if(!fp){
        perror("Erro ao abrir arquivo de entrada");
        exit(1);
    }

    int code[MAX_CODE_SIZE];
    int reloc[MAX_CODE_SIZE];
    for(int i = 0; i < MAX_CODE_SIZE; i++) {
        code[i]  = 0;
        reloc[i] = 0;
    }

    SymbolTable sym;
    sym.label_count   = 0;
    sym.pending_count = 0;

    int code_size       = 0;
    int current_section = 0; // 1=TEXT, 2=DATA
    int has_begin_end   = 0;

    char line[MAX_LINE_LENGTH];

    // ----------------------------------
    //  First pass: detect BEGIN
    // ----------------------------------
    while(fgets(line, sizeof(line), fp)){
        trim_newline(line);
        // If line has "BEGIN" in any case
        if(strcasestr(line, "BEGIN")) {
            has_begin_end = 1;
        }
    }
    rewind(fp);

    // ----------------------------------
    //           PASS 1
    // ----------------------------------
    code_size = 0;
    while(fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if(strlen(line) == 0) continue; // skip empty lines

        char *tk = strtok(line, " \t");
        if(!tk) continue;

        // Check if there's a label with ':'
        if(strchr(tk, ':')) {
            // Extract label name up to ':'
            char lbl[50];
            char *c = strchr(tk, ':');
            int len = (int)(c - tk);
            if(len <= 0 || len >= (int)sizeof(lbl)) {
                fprintf(stderr, "ERRO: Sintaxe de rótulo inválida '%s'.\n", tk);
                exit(1);
            }
            strncpy(lbl, tk, len);
            lbl[len] = '\0';

            // Peek the next token (in case it's EXTERN)
            char *lookahead = strtok(NULL, " \t");
            if(lookahead && strcasecmp(lookahead, "EXTERN") == 0) {
                // Mark as extern, not defined
                add_label(&sym, lbl, 0, 1, 0, 0);
                // Skip the rest of the line
                continue;
            }
            else {
                // Normal label definition at code_size
                add_label(&sym, lbl, code_size, 0, 0, 1);
                // Put the lookahead token back to process
                tk = lookahead;
                if(!tk) continue;
            }
        }

        if(!tk) continue;

        // Possibly "SECTION"
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

        // Possibly "PUBLIC", "EXTERN" (the case "EXTERN FOO")
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
            // e.g. "EXTERN FOO"
            char *lbl = strtok(NULL, " \t");
            if(lbl){
                add_label(&sym, lbl, 0, 1, 0, 0);
            }
            continue;
        }

        // Possibly "BEGIN"/"END"
        if(strcasecmp(tk, "BEGIN") == 0){
            // skip
            continue;
        }
        if(strcasecmp(tk, "END") == 0){
            continue;
        }

        // In TEXT, count instruction size
        if(current_section == 1) {
            int size;
            int op = find_opcode(tk, &size);
            if(op >= 0) {
                code_size += size;
            }
        }
        // In DATA, count directives
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

    // ----------------------------------
    //           PASS 2
    // ----------------------------------
    code_size = 0;
    current_section = 0;
    while(fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if(strlen(line) == 0) continue;

        char *tk = strtok(line, " \t");
        if(!tk) continue;

        // Label with colon?
        if(strchr(tk, ':')) {
            // we already processed the label in pass1, skip
            // read next token
            tk = strtok(NULL, " \t");
            if(!tk) continue;
        }

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

        // Skip PUBLIC / EXTERN
        if(strcasecmp(tk, "PUBLIC") == 0){
            strtok(NULL," \t");
            continue;
        }
        if(strcasecmp(tk, "EXTERN") == 0){
            strtok(NULL," \t");
            continue;
        }

        // Skip BEGIN/END
        if(strcasecmp(tk, "BEGIN") == 0){
            strtok(NULL," \t"); // possible module name
            continue;
        }
        if(strcasecmp(tk, "END") == 0){
            continue;
        }

        // If in TEXT, parse instructions
        if(current_section == 1) {
            int size = 0;
            int op = find_opcode(tk, &size);
            if(op >= 0) {
                // Store the opcode
                code[code_size] = op;
                reloc[code_size] = 0; // no reloc for opcode
                code_size++;

                // Next words = operands
                if(strcasecmp(tk, "COPY") == 0 && size == 3) {
                    // A single next token might be "X,Y"
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
                    // For other instructions, each operand is separate
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
        else if(current_section == 2) {
            // Data directives
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

    // Fix references
    fix_pending(&sym, code, code_size, reloc);

    // Write output file (we only open/close to finalize it)
    FILE *out = fopen(output_filename, "w");
    if(!out) {
        perror("Erro ao criar arquivo de saída");
        exit(1);
    }
    fclose(out);

    // If has BEGIN/END, print in module format
    if(has_begin_end){
        print_module_output(&sym, code, code_size, reloc);
    } else {
        print_flat_output(code, code_size);
    }

    printf("Montagem concluída. Saída: %s\n", output_filename);
}
