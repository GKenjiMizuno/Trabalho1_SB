#include <stdio.h>
#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#define MAX_LINES 1000

void preprocess_line(char *line);
void validate_copy(char *line);
void remove_extra_spaces(char *line);
void validate_const(char *line);
void preprocess_equ_if(char *line);
void associate_labels(char lines[MAX_LINES][256], int *line_count);
void reorder_sections(char lines[MAX_LINES][256], int line_count, FILE *output_file);

void preprocess_line(char *line);
void preprocess_file(const char *input_filename, const char *output_filename);


#endif