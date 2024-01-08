#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "type.h"
#include "messages.h"
#include "list.h"

// TODO we should check if the new type already exists to avoid duplicates
struct Type* pointer_to(struct Type* base) {
    struct Type *type = calloc(1, sizeof(struct Type));
    type->name = calloc(32, sizeof(char));
    sprintf(type->name, "%s*", base->name);
    type->kind = TY_POINTER;
    type->size = 2;
    type->base = base;
    type->parameters = NULL;
    return type;
}

struct Type* function_of(struct Type* base) {
    struct Type *type = calloc(1, sizeof(struct Type));
    type->name = calloc(32, sizeof(char));
    sprintf(type->name, "%s()", base->name);
    type->kind = TY_FUNC;
    type->size = 2;
    type->base = base;
    type->parameters = NULL;
    return type;
}

void add_parameter(struct Type* base, struct Type* new_parameter) {
    base->name[strlen(base->name)-1] = '\0'; // Erase last character aka ')'
    sprintf(base->name, "%s%s%s)", base->name, (base->parameters != NULL) ? "," : "", new_parameter->name);
    list_add(&base->parameters, new_parameter);
}

// Get lowest common denominator type
// These are essentially automatic casts
struct Type* get_common_type(struct Token* token, struct Type* left, struct Type* right) {
    // Promote char to int
    if ((left->kind == TY_INT) && (right->kind == TY_CHAR)) {
        return left;
    } else if ((left->kind == TY_CHAR) && (right->kind == TY_INT)) {
        return right;
    }
    
    // Any other mismatch is an error
    if (left->kind != right->kind) {
        error(token, "incompatible types '%s' and '%s'", left->name, right->name);
    }

    // Warn about changing pointer type
    if ((left->base != NULL) && (right->base != NULL)) {
        if (strcmp(left->base->name, right->base->name) != 0) {
            warning(token, "assignment of incompatible pointer types '%s' and '%s'", left->name, right->name);
        }
    }
    
    return left;
}