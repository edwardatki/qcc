#ifndef _SCOPE_H
#define _SCOPE_H

struct Symbol;
typedef struct Symbol Symbol;

struct Token;
typedef struct Token Token;

typedef struct Scope Scope;
typedef struct SymbolListEntry SymbolListEntry;

struct Scope {
    Scope* parentScope;
    int depth;
    int id;
    int stackSize;
    SymbolListEntry* symbolList;
};

struct SymbolListEntry {
    Symbol* symbol;
    SymbolListEntry* next;
};

void enterNewScope();
void exitScope();
Scope* getCurrentScope();
void scopeAddSymbol(Symbol*);
Symbol* lookupSymbol(Token*);
char* getSymbolLocation(Symbol*);
int getSymbolStackOffset(Symbol*, Scope*);

#endif