#ifndef _SYMBOL_H
#define _SYMBOL_H

struct Token;
typedef struct Token Token;

struct Type;
typedef struct Type Type;

typedef struct Symbol Symbol;

struct Symbol {
    Token* token;
    Type* type;
    int global;
    int stackPosition;
};

#endif