#ifndef _TYPE_H
#define _TYPE_H

enum TypeKind {TY_POINTER, TY_FUNC, TY_CHAR};

typedef struct Type Type;
struct Type {
    char* name;
    enum TypeKind kind;
    int size;
    Type* base;
};

static Type typeChar = {.name="char", .size=1};

static Type* baseTypes[] = {&typeChar};

static Type* pointerTo(Type* base) {
    Type *type = calloc(1, sizeof(Type));
    type->name = calloc(32, sizeof(char));
    sprintf(type->name, "%s*", base->name);
    type->kind = TY_POINTER;
    type->size = 2;
    type->base = base;
    return type;
}

#endif