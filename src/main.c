#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "generator.h"

int main() {
    FILE *fp;
    fp = fopen("test.s", "r");
    
    Token* firstToken = lex(fp);

    Node* rootNode = parse(firstToken);

    char* result = generate(rootNode);
    printf("%s", result);

    return 0;
}