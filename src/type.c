#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "type.h"
#include "messages.h"

static void add_type_list_entry(struct TypeListEntry** root_entry, struct Type* entryType) {
    if (*root_entry == NULL) {
        *root_entry = calloc(1, sizeof(struct TypeListEntry));
    }

    struct TypeListEntry* current_entry = *root_entry;

    *root_entry = current_entry;

    // Travel to end of list
    while (current_entry->next != NULL) {
        current_entry = current_entry->next;
    }

    // Create new entry
    struct TypeListEntry* new_entry = calloc(1, sizeof(struct TypeListEntry));
    new_entry->type = entryType;
    new_entry->next = NULL;
    current_entry->next = new_entry;
}

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

// TODO somehow keep track of parameters
struct Type* function_of(struct Type* base) {
    struct Type *type = calloc(1, sizeof(struct Type));
    type->name = calloc(32, sizeof(char));
    sprintf(type->name, "%s()", base->name);
    type->kind = TY_FUNC;
    type->size = 2;
    type->base = base;
    return type;
}

void add_parameter(struct Type* base, struct Type* new_parameter) {
    sprintf(base->name, "%s\b%s%s)", base->name, (base->parameters != NULL) ? "," : "", new_parameter->name);
    add_type_list_entry(&base->parameters, new_parameter);
}

// Get lowest common denominator type
// TODO surely I need to generate code to convert between types here, hmmmm
struct Type* get_common_type(struct Token* token, struct Type* left, struct Type* right) {
    // Promote int to pointer type but warn
    if ((left->kind == TY_POINTER) && (right->kind == TY_INT)) {
        warning(token, "assignment to '%s' from '%s' makes pointer from integer without a cast", left->name, right->name);
        return left;
    }

    // Promote char to int
    if ((left->kind == TY_INT) && (right->kind == TY_CHAR)) {
        return left;
    } else if ((left->kind == TY_CHAR) && (right->kind == TY_INT)) {
        return right;
    }
    
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