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

    currentScope = newScope;
    scopeCount += 1;
}

void exitScope() {
    Scope* oldScope = currentScope;
    currentScope = currentScope->parentScope;
    free(oldScope);
    // printf("EXIT SCOPE\n");
}

void scopeAddSymbol(Symbol* symbol) {
    addSymbolListEntry(&currentScope->symbolList, symbol);

    // TODO: Decide symbol location in memory
    symbol->location = calloc(32, sizeof(char));
    sprintf(symbol->location, "%s_%d", symbol->token->value, currentScope->id);
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
    exit(1);
}