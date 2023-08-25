#ifndef _REGISTER_H
#define _REGISTER_H

typedef struct Register Register;
struct Register {
    char* name;
    int size;
    int free;
    
    Register* subReg[2];
};

#endif