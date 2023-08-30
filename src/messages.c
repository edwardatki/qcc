#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "messages.h"
#include "lexer.h"

void error(Token* token, const char* format, ...) {
    if (token == NULL) {
        printf(RED "error: " RESET);
    } else {
        printf("%d:%d " RED "error: " RESET, token->line, token->column);
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");

    exit(EXIT_FAILURE);
}

void warning(Token* token, const char* format, ...) {
    if (token == NULL) {
        printf(YEL "warning: " RESET);
    } else {
        printf("%d:%d " YEL "warning: " RESET, token->line, token->column);
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}