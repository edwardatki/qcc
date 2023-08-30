#ifndef _TYPE_H
#define _TYPE_H

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

Type* pointerTo(Type*);
Type* getCommonType(Type*, Type*);

#endif