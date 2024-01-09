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

#endif