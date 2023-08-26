#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "generator.h"
#include "print_formatting.h"

int main() {
    FILE *fp;
    fp = fopen("test.s", "r");
    
    Token* firstToken = lex(fp);

    Node* rootNode = parse(firstToken);
    
    char* result = generate(rootNode);
    printf("\n--- OUTPUT ---\n");
    printf("%s", result);

    return 0;
}