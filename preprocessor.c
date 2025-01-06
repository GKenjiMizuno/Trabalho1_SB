#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"

#define MAX_LINES 1000

void preprocess_line(char *line) {
    char *comment = strchr(line, ';');
    if (comment) {
        *comment = '\0';
    }

    char *start = line;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    char *end = line + strlen(line) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    memmove(line, start, strlen(start) + 1);

    for (char *p = line; *p; p++) {
        *p = toupper((unsigned char)*p);
    }
}

void validate_copy(char *line) {
    if (strncmp(line, "COPY", 4) == 0) {
        char *comma = strchr(line, ',');
        if (!comma || *(comma + 1) == ' ') {
            fprintf(stderr, "Erro: COPY com argumentos mal formatados: %s\n", line);
            exit(1);
        }
    }
}

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

void validate_const(char *line) {
    if (strstr(line, "CONST")) {
        char *value = strstr(line, "CONST") + 6;
        while (isspace((unsigned char)*value)) {
            value++;
        }

        if (!isdigit(*value) && *value != '-' && strncmp(value, "0X", 2) != 0) {
            fprintf(stderr, "Erro: Valor inválido para CONST: %s\n", line);
            exit(1);
        }

        if (strncmp(value, "0X", 2) == 0) {
            value += 2;
            while (*value) {
                if (!isxdigit(*value)) {
                    fprintf(stderr, "Erro: Valor hexadecimal inválido para CONST: %s\n", line);
                    exit(1);
                }
                value++;
            }
        } else {
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

void preprocess_equ_if(char *line) {
    if (strstr(line, "EQU") || strstr(line, "IF")) {
        char *comment = strchr(line, ';');
        if (comment) {
            *comment = '\0';
        }
        remove_extra_spaces(line);
    }
}

void associate_labels(char lines[MAX_LINES][256], int *line_count) {
    for (int i = 0; i < *line_count - 1; i++) {
        char *colon = strchr(lines[i], ':');
        if (colon && strlen(colon + 1) == 0) {
            strcat(lines[i], " ");
            strcat(lines[i], lines[i + 1]);
            for (int j = i + 1; j < *line_count - 1; j++) {
                strcpy(lines[j], lines[j + 1]);
            }
            (*line_count)--;
        }
    }
}

void reorder_sections(char lines[MAX_LINES][256], int line_count, FILE *output_file) {
    char text_section[MAX_LINES][256];
    char data_section[MAX_LINES][256];
    int text_count = 0, data_count = 0;

    int current_section = 0;

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

    fprintf(output_file, "SECTION TEXT\n");
    for (int i = 0; i < text_count; i++) {
        if (strchr(text_section[i], ':')) {
            fprintf(output_file, "%s\n", text_section[i]);
        } else {
            fprintf(output_file, "    %s\n", text_section[i]);
        }
    }

    fprintf(output_file, "SECTION DATA\n");
    for (int i = 0; i < data_count; i++) {
        fprintf(output_file, "%s\n", data_section[i]);
    }
}