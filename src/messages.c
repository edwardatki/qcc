#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "messages.h"
#include "lexer.h"

// TODO some weirdness when token near EOF
static void print_token_context(Token* token, const char* color) {
    printf(WHT "%4d | ", token->line);
    char* line = get_line(token->line);
    int i = 0;
    int skip_whitespace = 0;
    while (1) {
        char c = line[i++];

        if ((c == '\n') || (c == EOF)) break;
        
        // Skip leading whitespace
        if (skip_whitespace && ((c == '\t') || (c == ' '))) continue;
        skip_whitespace = 0;

        if (i == token->column) printf(BOLD "%s", color);
        if (i == (token->column + token->length)) printf(RESET WHT);

        printf("%c", c);
    }
    printf(RESET "\n");
}

void error(Token* token, const char* format, ...) {
    if (token == NULL) {
        printf(BOLD RED "error: " RESET);
    } else {
        printf(BOLD "%s:%d:%d: " RED "error: " RESET, token->filename, token->line, token->column);
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");

    if (token != NULL) print_token_context(token, RED);

    exit(EXIT_FAILURE);
}

void warning(Token* token, const char* format, ...) {
    if (token == NULL) {
        printf(BOLD YEL "warning: " RESET);
    } else {
        printf(BOLD "%s:%d:%d: " YEL "warning: " RESET, token->filename, token->line, token->column);
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");

    if (token != NULL) print_token_context(token, YEL);
}