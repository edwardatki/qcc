#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "messages.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"

static struct Scope* current_scope = NULL;
static int scope_count = 0;

static void add_symbol_list_entry(struct SymbolListEntry** root_entry, struct Symbol* entry_symbol) {
    if (*root_entry == NULL) {
        *root_entry = calloc(1, sizeof(struct SymbolListEntry));
    }

    struct SymbolListEntry* current_entry = *root_entry;

    // Travel to end of list
    while (current_entry->next != NULL) {
        current_entry = current_entry->next;
    }

    // // Create new entry
    struct SymbolListEntry* new_entry = calloc(1, sizeof(struct SymbolListEntry));
    new_entry->symbol = entry_symbol;
    current_entry->next = new_entry;
}

void enter_new_scope() {
    // printf("ENTER SCOPE\n");
    struct Scope* new_scope = calloc(1, sizeof(struct Scope));
    new_scope->parent_scope = current_scope;
    new_scope->symbol_list = NULL;
    
    if (current_scope == NULL) new_scope->depth = 0;
    else new_scope->depth = current_scope->depth + 1;

    new_scope->id = scope_count;

    new_scope->stack_size = 0;

    current_scope = new_scope;
    scope_count += 1;
}

void exit_scope() {
    struct Scope* old_scope = current_scope;
    current_scope = current_scope->parent_scope;
    // printf("EXIT SCOPE\n");
}

struct Scope* get_current_scope() {
    return current_scope;
}

void scope_add_symbol(struct Symbol* symbol) {
    add_symbol_list_entry(&current_scope->symbol_list, symbol);

    // Check if in global scope
    if (current_scope->id == 0) {
        symbol->global = 1;
    } else {
        symbol->global = 0;

        // Allocate position on stack
        symbol->stack_position = current_scope->stack_size;
        current_scope->stack_size += symbol->type->size;
    }
}

// Finds the "most local" symbol of given identifier
struct Symbol* lookup_symbol(struct Token* token) {
    struct Scope* search_scope = current_scope;
    while (search_scope != NULL) {
        // printf("SEARCHING SCOPE: %d\n", search_scope->id);
        struct SymbolListEntry* current_entry = search_scope->symbol_list;
        while (current_entry != NULL) {
            if (current_entry->symbol != NULL) { // First item is always empty, should change how my lists work
                // Check if matches the symbol we're looking for
                // TODO: Type checking
                if (strcmp(current_entry->symbol->token->value, token->value) == 0) return current_entry->symbol;
            }
            current_entry = current_entry->next;
        }

        search_scope = search_scope->parent_scope;
    }

    error(token, "'%s' undeclared", token->value);
}

int get_symbol_stack_offset(struct Symbol* symbol, struct Scope* scope) {
    // Traverse up from scope until symbol located adding up all stack sizes
    // This should give us offsets for variables on parent stack frames

    struct Scope* search_scope = scope;
    int stack_offset = 0;
    while (search_scope != NULL) {
        // printf("SEARCHING SCOPE: %d size %d\n", search_scope->id, search_scope->stack_size);
        struct SymbolListEntry* current_entry = search_scope->symbol_list;
        while (current_entry != NULL) {
            if (current_entry->symbol != NULL) { // First item is always empty, should change how my lists work
                // Check if matches the symbol we're looking for
                if (strcmp(current_entry->symbol->token->value, symbol->token->value) == 0) {
                    // TODO make this clearer
                    return stack_offset+search_scope->stack_size-current_entry->symbol->stack_position-current_entry->symbol->type->size;
                }
            }
            current_entry = current_entry->next;
        }
        stack_offset += search_scope->stack_size;
        search_scope = search_scope->parent_scope;
    }

    return -1;
}