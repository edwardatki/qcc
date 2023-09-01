#include <stdlib.h>
#include <stdio.h>
#include "type.h"
#include "messages.h"

// TODO we should check if the new type already exists to avoid duplicates
struct Type* pointer_to(struct Type* base) {
    struct Type *type = calloc(1, sizeof(struct Type));
    type->name = calloc(32, sizeof(char));
    sprintf(type->name, "%s*", base->name);
    type->kind = TY_POINTER;
    type->size = 2;
    type->base = base;
    return type;
}

// TODO will have to determine a suitable type between left and right
struct Type* get_common_type(struct Type* left, struct Type* right) {
    if (left->kind != right->kind) {
        error(NULL, "incompatible types '%s' and '%s'", left->name, right->name);
    }
    
    return left;
}