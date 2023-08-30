#include <stdlib.h>
#include <stdio.h>
#include "type.h"
#include "messages.h"

// TODO we should check if the new type already exists to avoid duplicates
Type* pointerTo(Type* base) {
    Type *type = calloc(1, sizeof(Type));
    type->name = calloc(32, sizeof(char));
    sprintf(type->name, "%s*", base->name);
    type->kind = TY_POINTER;
    type->size = 2;
    type->base = base;
    return type;
}

// TODO will have to determine a suitable type between left and right
Type* getCommonType(Type* left, Type* right) {
    if (left->kind != right->kind) {
        error(NULL, "incompatible types '%s' and '%s'", left->name, right->name);
    }
    
    return left;
}