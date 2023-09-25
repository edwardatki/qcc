#ifndef _TYPE_H
#define _TYPE_H

struct Token;

enum TypeKind {TY_VOID, TY_POINTER, TY_FUNC, TY_CHAR};

struct Type {
    char* name;
    enum TypeKind kind;
    int size;
    struct Type* base;
};

static struct Type type_void = {.name="void", .kind=TY_VOID, .size=0};
static struct Type type_char = {.name="char", .kind=TY_CHAR, .size=1};

static struct Type* base_types[] = {&type_void, &type_char};

struct Type* pointer_to(struct Type*);
struct Type* get_common_type(struct Token*, struct Type*, struct Type*);

#endif