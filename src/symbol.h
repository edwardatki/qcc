#ifndef _SYMBOL_H
#define _SYMBOL_H

#include "lexer.h"

typedef struct Type Type;
struct Type {
    char* name;
    int size;
};

static Type builtinTypes[] = {{.name="uint8", .size=1}};

typedef struct Symbol Symbol;
struct Symbol {
    Token* token;
    Type* type;
    char* memoryLocation;
};

typedef struct SymbolList SymbolList;
struct SymbolList {
    Symbol* symbol;
    SymbolList* next;
};

#endif