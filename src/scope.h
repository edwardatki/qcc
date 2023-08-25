#ifndef _SCOPE_H
#define _SCOPE_H

#include "symbol.h"
#include "parser.h"

typedef struct Scope Scope;
typedef struct SymbolListEntry SymbolListEntry;

struct Scope {
    Scope* parentScope;
    int depth;
    int id;
    SymbolListEntry* symbolList;
};

struct SymbolListEntry {
    Symbol* symbol;
    SymbolListEntry* next;
};

void enterNewScope();
void exitScope();
void scopeAddSymbol(Symbol*);
Symbol* lookupSymbol(Token*);

#endif