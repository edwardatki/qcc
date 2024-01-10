#include <stdio.h>
#include "register.h"
#include "messages.h"

struct Register registers[] = { {.name="a", .size=1, .free=1, .parent_reg=NULL, .high_reg=NULL, .low_reg=NULL},
                                {.name="b", .size=1, .free=1, .parent_reg=&registers[5], .high_reg=NULL, .low_reg=NULL},
                                {.name="c", .size=1, .free=1, .parent_reg=&registers[5], .high_reg=NULL, .low_reg=NULL},
                                {.name="d", .size=1, .free=1, .parent_reg=&registers[6], .high_reg=NULL, .low_reg=NULL},
                                {.name="e", .size=1, .free=1, .parent_reg=&registers[6], .high_reg=NULL, .low_reg=NULL},
                                {.name="bc", .size=2, .free=1, .parent_reg=NULL, .high_reg=&registers[1], .low_reg=&registers[2]},
                                {.name="de", .size=2, .free=1, .parent_reg=NULL, .high_reg=&registers[3], .low_reg=&registers[4]}};

void dump_register_usage() {
    printf("a: %s\n", registers[0].free ? GRN "free" RESET : RED "used" RESET);
    printf("b: %s\n", registers[1].free ? GRN "free" RESET : RED "used" RESET);
    printf("c: %s\n", registers[2].free ? GRN "free" RESET : RED "used" RESET);
    printf("d: %s\n", registers[3].free ? GRN "free" RESET : RED "used" RESET);
    printf("e: %s\n", registers[4].free ? GRN "free" RESET : RED "used" RESET);
    printf("bc: %s\n", registers[5].free ? GRN "free" RESET : RED "used" RESET);
    printf("de: %s\n", registers[6].free ? GRN "free" RESET : RED "used" RESET);
}

struct Register* allocate_reg(int size) {
    for (int i = 0; i < sizeof(registers)/sizeof(struct Register); i++) {
        // if (i == 0) continue; // TEMPORARY don't allocate a so we never have to push it to the stack
        struct Register* reg = &registers[i];
        if ((reg->size >= size) && reg->free) {
            // If sub registers are in use then can't allocate
            if ((reg->high_reg != NULL) && !reg->high_reg->free) continue;
            if ((reg->low_reg != NULL) && !reg->low_reg->free) continue;

            // Mark register as used
            reg->free = 0;

            // Mark sub registers as used too
            if (reg->high_reg != NULL) reg->high_reg->free = 0;
            if (reg->low_reg != NULL) reg->low_reg->free = 0;

            // printf("ALLOCATED REGISTER %s\n", reg->name);
            return reg;
        }
    }

    dump_register_usage();
    error(NULL, "unable to allocate register of size %d", size);
}

void free_reg(struct Register* reg) {
    if (reg == NULL) return;

    // Mark register as free
    reg->free = 1;

    // Mark sub registers as free too
    if (reg->high_reg != NULL) reg->high_reg->free = 1;
    if (reg->low_reg != NULL) reg->low_reg->free = 1;

    // printf("FREED REGISTER %s\n", reg->name);
}

void free_reg_no_sub(struct Register* reg) {
    if (reg == NULL) return;

    // Mark register as free
    reg->free = 1;

    // printf("FREED REGISTER %s\n", reg->name);
}