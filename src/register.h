#ifndef _REGISTER_H
#define _REGISTER_H

struct Register {
    char* name;
    int size;
    int free;
    
    struct Register* sub_reg[2];
};

#endif