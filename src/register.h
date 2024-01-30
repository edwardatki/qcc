#ifndef _REGISTER_H
#define _REGISTER_H

struct Register {
    char* name;
    int size;
    int free;
    
    struct Register* parent_reg;
    struct Register* high_reg;
    struct Register* low_reg;
};

extern struct Register registers[];

enum {
    REG_A = 0,
    REG_B = 1,
    REG_C = 2,
    REG_D = 3,
    REG_E = 4,
    REG_BC = 5,
    REG_DE = 6
};

void dump_register_usage();
struct Register* allocate_reg(int);
void free_reg(struct Register*);
void free_reg_no_sub(struct Register*);

#endif