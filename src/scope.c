#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "messages.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"
#include "list.h"

static struct Scope* current_scope = NULL;
static int scope_count = 0;

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

// Check if symbol had already been declared in the current scope
int declared_in_current_scope(struct Token* target_token) {
    if (current_scope->symbol_list != NULL) {
        struct List* current_entry = current_scope->symbol_list;
        do {
            struct Symbol* current_symbol = (struct Symbol*)current_entry->value;

            // Check if matches the symbol we're looking for
            if (strcmp(current_symbol->token->value, target_token->value) == 0) {
                return 1;
            }
        } while (list_next(&current_entry));
    }

    return 0;
}

// Add symbol to current scope
void scope_add_symbol(struct Symbol* symbol) {
    if (declared_in_current_scope(symbol->token)) error(symbol->token, "symbol already declared");

    list_add(&current_scope->symbol_list, symbol);

    // Check if in global scope
    if (current_scope->id == 0) {
        symbol->global = 1;
    } else {
        symbol->global = 0;

        // Allocate position on stack
        if (!symbol->is_extern) {
            symbol->stack_position = current_scope->stack_size;
            current_scope->stack_size += symbol->type->size;
        }
    }
}


// Finds the "most local" symbol of given identifier
struct Symbol* lookup_symbol(struct Token* target_token) {
    struct Scope* search_scope = current_scope;
    while (search_scope != NULL) {
        // printf("SEARCHING SCOPE: %d for %s\n", search_scope->id, token->value);
        // If scope has contains symbols then search it
        if (search_scope->symbol_list != NULL) {
            struct List* current_entry = search_scope->symbol_list;
            do {
                struct Symbol* current_symbol = (struct Symbol*)current_entry->value;

                // Check if matches the symbol we're looking for
                // printf(" test: %s\n", symbol->token->value);
                if (strcmp(current_symbol->token->value, target_token->value) == 0) {
                    return current_symbol;
                }
            } while (list_next(&current_entry));
        }

        // Not found so move up to parent scope
        search_scope = search_scope->parent_scope;
    }

    error(target_token, "'%s' undeclared", target_token->value);
}

int get_symbol_stack_offset(struct Symbol* target_symbol, struct Scope* scope) {
    // Traverse up from scope until symbol located adding up all stack sizes
    // This should give us offsets for variables on parent stack frames

    struct Scope* search_scope = scope;
    int stack_offset = 0;
    while (search_scope != NULL) {
        // printf("SEARCHING SCOPE: %d size %d\n", search_scope->id, search_scope->stack_size);
        // If scope has contains symbols then search it
        if (search_scope->symbol_list != NULL) {
            struct List* current_entry = search_scope->symbol_list;
            do {
                struct Symbol* current_symbol = (struct Symbol*)current_entry->value;

                // Check if matches the symbol we're looking for
                if (strcmp(current_symbol->token->value, target_symbol->token->value) == 0) {
                    // TODO make this clearer
                    return stack_offset + search_scope->stack_size - current_symbol->stack_position - target_symbol->type->size;
                }
            } while (list_next(&current_entry));
        }

        stack_offset += search_scope->stack_size;
        search_scope = search_scope->parent_scope;
    }

    return -1;
}