#ifndef _SNIPPETS_H
#define _SNIPPETS_H

void add_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tadd %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
}

void sub_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tsub %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
}

void and_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tand %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
}

void or_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tor %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
}

void cmp_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tcmp %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
}

void zero_u8(FILE* fp, struct Register* left_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tcmp 0\n");
    fprintf(fp, "\tlde\n");
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
}

void equal_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tcmp %s\n", right_reg->name);
    fprintf(fp, "\tlde\n");
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
}

#endif