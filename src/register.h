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

void dump_register_usage();
struct Register* allocate_reg(int);
void free_reg(struct Register*);
void free_reg_no_sub(struct Register*);

#endif