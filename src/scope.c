#include "scope.h"
#include "print_formatting.h"

static Scope* currentScope = NULL;
static int scopeCount = 0;

static void addSymbolListEntry(SymbolListEntry** rootEntry, Symbol* entrySymbol) {
    if (*rootEntry == NULL) {
        *rootEntry = calloc(1, sizeof(SymbolListEntry));
    }

    SymbolListEntry* curEntry = *rootEntry;

    // Travel to end of list
    while (curEntry->next != NULL) {
        curEntry = curEntry->next;
    }

    // // Create new entry
    SymbolListEntry* newEntry = calloc(1, sizeof(SymbolListEntry));
    newEntry->symbol = entrySymbol;
    curEntry->next = newEntry;
}

void enterNewScope() {
    // printf("ENTER SCOPE\n");
    Scope* newScope = calloc(1, sizeof(Scope));
    newScope->parentScope = currentScope;
    newScope->symbolList = NULL;
    
    if (currentScope == NULL) newScope->depth = 0;
    else newScope->depth = currentScope->depth + 1;

    newScope->id = scopeCount;

    newScope->stackSize = 0;

    currentScope = newScope;
    scopeCount += 1;
}

void exitScope() {
    Scope* oldScope = currentScope;
    currentScope = currentScope->parentScope;
    // printf("EXIT SCOPE\n");
}

Scope* getCurrentScope() {
    return currentScope;
}

void scopeAddSymbol(Symbol* symbol) {
    addSymbolListEntry(&currentScope->symbolList, symbol);

    // Check if in global scope
    if (currentScope->id == 0) {
        symbol->global = 1;
    } else {
        symbol->global = 0;

        // Allocate position on stack
        symbol->stackPosition = currentScope->stackSize;
        currentScope->stackSize += symbol->type->size;
    }
}

// Finds the "most local" symbol of given identifier
Symbol* lookupSymbol(Token* token) {
    Scope* searchScope = currentScope;
    while (searchScope != NULL) {
        // printf("SEARCHING SCOPE: %d\n", searchScope->id);
        SymbolListEntry* curEntry = searchScope->symbolList;
        while (curEntry != NULL) {
            if (curEntry->symbol != NULL) { // First item is always empty, should change how my lists work
                // Check if matches the symbol we're looking for
                // TODO: Type checking
                if (strcmp(curEntry->symbol->token->value, token->value) == 0) return curEntry->symbol;
            }
            curEntry = curEntry->next;
        }

        searchScope = searchScope->parentScope;
    }

    printf("%d:%d %serror:%s '%s' undeclared\n", token->line, token->column, RED, RESET, token->value);
    exit(EXIT_FAILURE);
}

int getSymbolStackOffset(Symbol* symbol, Scope* scope) {
    // Traverse up from scope until symbol located adding up all stack sizes
    // This should give us offsets for variables on parent stack frames

    Scope* searchScope = scope;
    int stackOffset = 0;
    while (searchScope != NULL) {
        // printf("SEARCHING SCOPE: %d size %d\n", searchScope->id, searchScope->stackSize);
        SymbolListEntry* curEntry = searchScope->symbolList;
        while (curEntry != NULL) {
            if (curEntry->symbol != NULL) { // First item is always empty, should change how my lists work
                // Check if matches the symbol we're looking for
                if (strcmp(curEntry->symbol->token->value, symbol->token->value) == 0) {
                    // TODO make this clearer
                    return stackOffset+searchScope->stackSize-curEntry->symbol->stackPosition-curEntry->symbol->type->size;
                }
            }
            curEntry = curEntry->next;
        }
        stackOffset += searchScope->stackSize;
        searchScope = searchScope->parentScope;
    }

    return -1;
}