#ifndef _SYMBOL_H
#define _SYMBOL_H

#include "lexer.h"
#include "type.h"

typedef struct Symbol Symbol;

struct Symbol {
    Token* token;
    Type* type;
    int global;
    int stackPosition;
};

#endif