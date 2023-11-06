#include <stdlib.h>
#include "list.h"

void list_add(struct List** list_root, void* new_value) {
    // Create root entry if it doesn't exist
    if (*list_root == NULL) {
        *list_root = calloc(1, sizeof(struct List));
        (*list_root)->value = new_value;
        (*list_root)->next = NULL;
        return;
    }

    // Travel to end of list
    struct List* current_entry = *list_root;
    while (current_entry->next != NULL) {
        current_entry = current_entry->next;
    }

    // Create new entry
    struct List* new_entry = calloc(1, sizeof(struct List));
    new_entry->value = new_value;
    new_entry->next = NULL;
    current_entry->next = new_entry;
}

int list_next(struct List** list_entry) {
    if (*list_entry == NULL) return 0;
    if ((*list_entry)->next == NULL) return 0;

    *list_entry = (*list_entry)->next;
    return 1;
}