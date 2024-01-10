#ifndef _SYMBOL_H
#define _SYMBOL_H

struct Token;
struct Type;

struct Symbol {
    struct Token* token;
    struct Type* type;
    int global;
    int stack_position;
    int is_extern;
};

#endif