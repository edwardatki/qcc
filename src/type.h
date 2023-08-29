#ifndef _TYPE_H
#define _TYPE_H

#include "print_formatting.h"

enum TypeKind {TY_VOID, TY_POINTER, TY_FUNC, TY_CHAR};

typedef struct Type Type;
struct Type {
    char* name;
    enum TypeKind kind;
    int size;
    Type* base;
};

static Type typeVoid = {.name="void", .kind=TY_VOID, .size=0};
static Type typeChar = {.name="char", .kind=TY_CHAR, .size=1};

static Type* baseTypes[] = {&typeVoid, &typeChar};

// TODO we should check if the new type already exists to avoid duplicates
static Type* pointerTo(Type* base) {
    Type *type = calloc(1, sizeof(Type));
    type->name = calloc(32, sizeof(char));
    sprintf(type->name, "%s*", base->name);
    type->kind = TY_POINTER;
    type->size = 2;
    type->base = base;
    return type;
}

// TODO will have to determine a suitable type between left and right
static Type* getCommonType(Type* left, Type* right) {
    if (left->kind != right->kind) {
        printf("%serror:%s incompatible types\n", RED, RESET);
        exit(EXIT_FAILURE);
    }
    
    return left;
}

#endif