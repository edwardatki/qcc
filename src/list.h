#ifndef _LIST_H
#define _LIST_H

struct List {
    void* value;
    struct List* next;
};

void list_add(struct List**, void*);
int list_next(struct List**);

#endif