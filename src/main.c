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
            // TODO handle multiple input files
            if (input_filename != NULL) error(NULL, "more than one input file supplied");
            input_filename = argv[i];
            i += 1;
        }
    }
    
    if (input_filename == NULL) error(NULL, "no input file supplied");
    if (output_filename == NULL) {
        // Attempt to set filename to input name but changed to .asm
        output_filename = calloc(strlen(input_filename)+3, sizeof(char));
        strcpy(output_filename, input_filename);
        char *p = strstr(output_filename, ".");
        if (p != NULL) {
            memcpy(p, ".asm", 4);
        } else {
            free(output_filename);
            output_filename = "out.asm";
        }
    }
    
    // printf("%s -> %s\n", input_filename, output_filename);

    // Lex
    struct Token* first_token = lex(input_filename);
    
    // Parse
    struct Node* root_node = parse(first_token);
    
    // Generate code
    generate(root_node, output_filename);

    return EXIT_SUCCESS;
}

