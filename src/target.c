#include <stdio.h>
#include <string.h>
#include "target.h"
#include "register.h"
#include "messages.h"

void emit_label(FILE* fp, char* label, int count) {
    if (count < 0) fprintf(fp, "%s:\n", label);
    else fprintf(fp, "%s_%d:\n", label, count);
}

void emit_move(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    fprintf(fp, "\tmov %s, %s\n", left_reg->name, right_reg->name);
}

void emit_push(FILE* fp, struct Register* left_reg) {
    fprintf(fp, "\tpush %s\n", left_reg->name);
    // free_reg(left_reg);
}

void emit_pop(FILE* fp, struct Register* left_reg) {
    fprintf(fp, "\tpop %s\n", left_reg->name);
    // registers[0].free = 0;
}

void emit_stack_reserve(FILE* fp, unsigned int size) {
    fprintf(fp, "\tmov sp, sp-%d\n", size);
}

void emit_stack_free(FILE* fp, unsigned int size) {
    fprintf(fp, "\tmov sp, sp+%d\n", size);
}

void emit_immediate_load(FILE* fp, struct Register* reg, char* value) {
    fprintf(fp, "\tmov %s, %s\n", reg->name, value);
};

void emit_indirect_load(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (right_reg->size != 2) error(NULL, "indirect load must be a 16-bit register");
    fprintf(fp, "\tmov %s, [%s]\n", left_reg->name, right_reg->name);
}

void emit_indirect_store(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size != 2) error(NULL, "indirect store must be a 16-bit register");
    fprintf(fp, "\tmov [%s], %s\n", left_reg->name, right_reg->name);
}

void emit_call(FILE* fp, char* label, int count) {
    if (count < 0) fprintf(fp, "\tcall %s\n", label);
    else fprintf(fp, "\tcall %s_%d\n", label, count);
}

void emit_jump(FILE* fp, char* label, int count) {
    if (count < 0) fprintf(fp, "\tjmp %s\n", label);
    else fprintf(fp, "\tjmp %s_%d\n", label, count);
}

void emit_jump_if_equal(FILE* fp, char* label, int count) {
    if (count < 0) fprintf(fp, "\tje %s\n", label);
    else fprintf(fp, "\tje %s_%d\n", label, count);
}

void emit_jump_if_not_equal(FILE* fp, char* label, int count) {
    if (count < 0) fprintf(fp, "\tjne %s\n", label);
    else fprintf(fp, "\tjne %s_%d\n", label, count);
}

void emit_jump_if_carry(FILE* fp, char* label, int count) {
    if (count < 0) fprintf(fp, "\tjc %s\n", label);
    else fprintf(fp, "\tjc %s_%d\n", label, count);
}

void emit_jump_if_not_carry(FILE* fp, char* label, int count) {
    if (count < 0) fprintf(fp, "\tjnc %s\n", label);
    else fprintf(fp, "\tjnc %s_%d\n", label, count);
}

void emit_return(FILE* fp) {
    fprintf(fp, "\tret\n");
}

void emit_cmp_zero(FILE* fp) {
    fprintf(fp, "\tcmp 0\n");
};

void emit_add_immediate(FILE* fp, struct Register* left_reg, int value) {
    if (value == 1) {
        fprintf(fp, "\tinc %s\n", left_reg->name);
    } else {
        // Preserve accumulator if necessary
        if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) emit_push(fp, &registers[REG_A]);
        
        // Move to accumulator if necessary
        if (strcmp(left_reg->name, "a") != 0) emit_move(fp, &registers[REG_A], left_reg);

        fprintf(fp, "\tadd %d\n", value);

        // Restore accumulator if necessary
        if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) emit_pop(fp, &registers[REG_A]);
    }
};

void emit_sub_immediate(FILE* fp, struct Register* left_reg, int value) {
    if (value == 1) {
        fprintf(fp, "\tdec %s\n", left_reg->name);
    } else {
        // Preserve accumulator if necessary
        if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) emit_push(fp, &registers[REG_A]);
        
        // Move to accumulator if necessary
        if (strcmp(left_reg->name, "a") != 0) emit_move(fp, &registers[REG_A], left_reg);

        fprintf(fp, "\tsub %d\n", value);

        // Restore accumulator if necessary
        if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) emit_pop(fp, &registers[REG_A]);
    }
};

static struct Register* add_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tadd %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);
    return left_reg;
}

static struct Register* add_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    fprintf(fp, "add16 %s, %s\n", left_reg->name, right_reg->name);
    free_reg(right_reg);
    return left_reg;
}

struct Register* emit_add(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return add_u8(fp, left_reg, right_reg);
    else return add_u16(fp, left_reg, right_reg);
}

static struct Register* sub_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tsub %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);
    return left_reg;
}

static struct Register* sub_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    fprintf(fp, "sub16 %s, %s\n", left_reg->name, right_reg->name);
    free_reg(right_reg);
    return left_reg;
}

struct Register* emit_sub(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return sub_u8(fp, left_reg, right_reg);
    else return sub_u16(fp, left_reg, right_reg);
}

static struct Register* and_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tand %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);
    return left_reg;
}

static struct Register* and_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    and_u8(fp, left_reg->low_reg, right_reg->low_reg);
    and_u8(fp, left_reg->high_reg, right_reg->high_reg);
    free_reg(right_reg);
    return left_reg;
}

struct Register* emit_and(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return and_u8(fp, left_reg, right_reg);
    else return and_u16(fp, left_reg, right_reg);
}

static struct Register* or_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tor %s\n", right_reg->name);
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);
    return left_reg;
}

static struct Register* or_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    or_u8(fp, left_reg->low_reg, right_reg->low_reg);
    or_u8(fp, left_reg->high_reg, right_reg->high_reg);
    free_reg(right_reg);
    return left_reg;
}

struct Register* emit_or(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return or_u8(fp, left_reg, right_reg);
    else return or_u16(fp, left_reg, right_reg);
}

static struct Register* is_zero_u8(FILE* fp, struct Register* left_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tcmp 0\n");
    fprintf(fp, "\tlde\n");
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    return left_reg;
}

struct Register* emit_is_zero(FILE* fp, struct Register* left_reg) {
    if (left_reg->size == 1) return is_zero_u8(fp, left_reg);
    else error(NULL, "emit is_zero_u16 not implemented");
}

static struct Register* is_equal_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);
    fprintf(fp, "\tcmp %s\n", right_reg->name);
    fprintf(fp, "\tlde\n");
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);
    return left_reg;
}

static struct Register* is_equal_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    is_equal_u8(fp, left_reg->low_reg, right_reg->low_reg);
    is_equal_u8(fp, left_reg->high_reg, right_reg->high_reg);
    free_reg(right_reg);

    and_u8(fp, left_reg->high_reg, left_reg->low_reg);
    free_reg_no_sub(left_reg->low_reg);
    free_reg_no_sub(left_reg);
    return left_reg->high_reg;
}

struct Register* emit_is_equal(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return is_equal_u8(fp, left_reg, right_reg);
    else is_equal_u16(fp, left_reg, right_reg);
}

struct Register* emit_is_not_equal(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    left_reg = emit_is_equal(fp, left_reg, right_reg);
    left_reg = emit_is_zero(fp, left_reg);
    return left_reg;
}

static struct Register* is_more_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);

    fprintf(fp, "\tcmp %s\n", right_reg->name);
    fprintf(fp, "\tldc\n"); // A <= right

    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);

    return left_reg;
}

static struct Register* is_more_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    // TODO
    error(NULL, "emit is_more_u16 not implemented");

    free_reg(right_reg);
    free_reg_no_sub(left_reg->high_reg);
    free_reg_no_sub(left_reg);

    return left_reg->low_reg;
}

struct Register* emit_is_more(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return is_more_u8(fp, left_reg, right_reg);
    else return is_more_u16(fp, left_reg, right_reg);
}

static struct Register* is_more_or_equal_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);

    static int label_count = 0;

    fprintf(fp, "\tcmp %s\n", right_reg->name);
    fprintf(fp, "\tje .cmp_more_equal_u8_true_%d\n", label_count); // A == right
    fprintf(fp, "\tjnc .cmp_more_equal_u8_false_%d\n", label_count); // A <= right

    fprintf(fp, ".cmp_more_equal_u8_true_%d:\n", label_count);
    fprintf(fp, "\tmov a, 1\n");
    fprintf(fp, "\tjmp .cmp_more_equal_u8_exit_%d\n", label_count);

    fprintf(fp, ".cmp_more_equal_u8_false_%d:\n", label_count);
    fprintf(fp, "\tmov a, 0\n");

    fprintf(fp, ".cmp_more_equal_u8_exit_%d:\n", label_count);

    label_count++;

    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);

    return left_reg;
}

static struct Register* is_more_or_equal_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    // TODO
    error(NULL, "emit is_more_or_equal_u16 not implemented");

    free_reg(right_reg);
    free_reg_no_sub(left_reg->high_reg);
    free_reg_no_sub(left_reg);

    return left_reg->low_reg;
}

struct Register* emit_is_more_or_equal(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return is_more_or_equal_u8(fp, left_reg, right_reg);
    else return is_more_or_equal_u16(fp, left_reg, right_reg);
}

static struct Register* is_less_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);

    static int label_count = 0;

    fprintf(fp, "\tcmp %s\n", right_reg->name);
    fprintf(fp, "\tjc .cmp_less_u8_false_%d\n", label_count);  // A > right
    fprintf(fp, "\tje .cmp_less_u8_false_%d\n", label_count);  // A == right

    fprintf(fp, ".cmp_less_u8_true_%d:\n", label_count);
    fprintf(fp, "\tmov a, 1\n");
    fprintf(fp, "\tjmp .cmp_less_u8_exit_%d\n", label_count);

    fprintf(fp, ".cmp_less_u8_false_%d:\n", label_count);
    fprintf(fp, "\tmov a, 0\n");

    fprintf(fp, ".cmp_less_u8_exit_%d:\n", label_count);

    label_count++;

    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);

    return left_reg;
}

static struct Register* is_less_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    // TODO
    error(NULL, "emit is_less_u16 not implemented");

    free_reg(right_reg);
    free_reg_no_sub(left_reg->high_reg);
    free_reg_no_sub(left_reg);

    return left_reg->low_reg;
}

struct Register* emit_is_less(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return is_less_u8(fp, left_reg, right_reg);
    else is_less_u16(fp, left_reg, right_reg);
}

static struct Register* is_less_or_equal_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);

    static int label_count = 0;

    fprintf(fp, "\tcmp %s\n", right_reg->name);
    fprintf(fp, "\tjc .cmp_less_equal_u8_false_%d\n", label_count); // A > right

    fprintf(fp, ".cmp_less_equal_u8_true_%d:\n", label_count);
    fprintf(fp, "\tmov a, 1\n");
    fprintf(fp, "\tjmp .cmp_less_equal_u8_exit_%d\n", label_count);

    fprintf(fp, ".cmp_less_equal_u8_false_%d:\n", label_count);
    fprintf(fp, "\tmov a, 0\n");

    fprintf(fp, ".cmp_less_equal_u8_exit_%d:\n", label_count);

    label_count++;

    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);
    
    return left_reg;
}

static struct Register* is_less_or_equal_u16(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    // TODO
    error(NULL, "emit is_less_or_equal_u16 not implemented");

    free_reg(right_reg);
    free_reg_no_sub(left_reg->high_reg);
    free_reg_no_sub(left_reg);

    return left_reg->low_reg;
}

struct Register* emit_is_less_or_equal(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return is_less_or_equal_u8(fp, left_reg, right_reg);
    else return is_less_or_equal_u16(fp, left_reg, right_reg);
}

// TODO this is a rotate instead of shift :(
static struct Register* left_shift_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);

    static int label_count = 0;

    fprintf(fp, "\tpush a\n");
    fprintf(fp, "\tmov a, %s\n", right_reg->name);
    fprintf(fp, "\tand 0b111\n");
    fprintf(fp, "\tmov %s, a\n", right_reg->name);
    fprintf(fp, "\tpop a\n");

    fprintf(fp, ".shl_loop_%d:\n", label_count);
    fprintf(fp, "\tdec %s\n", right_reg->name);
    fprintf(fp, "\tjnc .shl_exit_%d\n", label_count);
    fprintf(fp, "\trol\n");
    fprintf(fp, "\tjmp .shl_loop_%d\n", label_count);
    fprintf(fp, ".shl_exit_%d:\n", label_count);

    label_count++;

    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);

    return left_reg;
}

struct Register* emit_left_shift(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return left_shift_u8(fp, left_reg, right_reg);
    else error(NULL, "emit shift_left_u16 not implemented");
}

// TODO this is a rotate instead of shift :(
static struct Register* right_shift_u8(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);

    static int label_count = 0;

    fprintf(fp, "\tpush a\n");
    fprintf(fp, "\tmov a, 8\n");
    fprintf(fp, "\tsub %s\n", right_reg->name);
    fprintf(fp, "\tand 0b111\n");
    fprintf(fp, "\tmov %s, a\n", right_reg->name);
    fprintf(fp, "\tpop a\n");

    fprintf(fp, ".shr_loop_%d:\n", label_count);
    fprintf(fp, "\tdec %s\n", right_reg->name);
    fprintf(fp, "\tjnc .shr_exit_%d\n", label_count);
    fprintf(fp, "\trol\n");
    fprintf(fp, "\tjmp .shr_loop_%d\n", label_count);
    fprintf(fp, ".shr_exit_%d:\n", label_count);

    label_count++;

    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);
    free_reg(right_reg);

    return left_reg;
}

struct Register* emit_right_shift(FILE* fp, struct Register* left_reg, struct Register* right_reg) {
    if (left_reg->size == 1) return right_shift_u8(fp, left_reg, right_reg);
    else error(NULL, "emit shift_right_u16 not implemented");
}