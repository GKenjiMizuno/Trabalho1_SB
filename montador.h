#ifndef MONTADOR_H
#define MONTADOR_H

typedef struct {
    char mnemonico[10];
    int opcode;
    int tamanho;
} OpCode;

void montar_programa(const char *input_filename, const char *output_filename);

#endif // MONTADOR_H
