#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "generator.h"
#include "lexer.h"
#include "messages.h"
#include "parser.h"

int main(int argc, char **argv) {
    char* input_filename = NULL;
    char* output_filename = NULL;
    
    // Process arguments
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-o") == 0) {
            if (argc <= (i+1)) error(NULL, "flag given with no value");
            output_filename = argv[i+1];
            i += 2;
        } else {
            if (input_filename != NULL) error(NULL, "more than one input file supplied");
            input_filename = argv[i];
            i += 1;
        }
    }
    
    if (input_filename == NULL) error(NULL, "no input file supplied");

    // Lex
    struct Token* first_token = lex(input_filename);
    
    // Parse
    struct Node* root_node = parse(first_token);
    
    // Generate code
    char* result = generate(root_node);

    // Write to output file
    if (output_filename == NULL) output_filename = "out.asm";
    FILE *output_file;
    output_file = fopen(output_filename, "w");
    fprintf(output_file, "%s", result);
    fclose(output_file);

    return EXIT_SUCCESS;
}