#ifndef _SCOPE_H
#define _SCOPE_H

struct Symbol;
struct Token;

struct Scope {
    struct Scope* parent_scope;
    int depth;
    int id;
    int stack_size;
    struct SymbolListEntry* symbol_list;
};

struct SymbolListEntry {
    struct Symbol* symbol;
    struct SymbolListEntry* next;
};

void enter_new_scope();
void exit_scope();
struct Scope* get_current_scope();
void scope_add_symbol(struct Symbol*);
struct Symbol* lookup_symbol(struct Token*);
int get_symbol_stack_offset(struct Symbol*, struct Scope*);

#endif