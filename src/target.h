#ifndef _TARGET_H
#define _TARGET_H

struct _IO_FILE;
typedef struct _IO_FILE FILE;

struct Register;

void emit_label(FILE* fp, char*, int);
void emit_move(FILE*, struct Register*, struct Register*);
void emit_push(FILE*, struct Register*);
void emit_pop(FILE*, struct Register*);
void emit_stack_reserve(FILE*, unsigned int);
void emit_stack_free(FILE*, unsigned int);

void emit_immediate_load(FILE*, struct Register*, char*);
// void emit_absolute_load(FILE*, struct Register*, char*);
// void emit_absolute_store(FILE*, char*, struct Register*);
void emit_indirect_load(FILE*, struct Register*, struct Register*);
void emit_indirect_store(FILE*, struct Register*, struct Register*);

void emit_call(FILE*, char*, int);
void emit_jump(FILE*, char*, int);
void emit_jump_if_equal(FILE*, char*, int);
void emit_jump_if_not_equal(FILE*, char*, int);
void emit_jump_if_carry(FILE*, char*, int);
void emit_jump_if_not_carry(FILE*, char*, int);

void emit_return(FILE*);
void emit_cmp_zero(FILE*);

void emit_add_immediate(FILE*, struct Register*, int);
void emit_sub_immediate(FILE*, struct Register*, int);

struct Register* emit_add(FILE*, struct Register*, struct Register*);
struct Register* emit_sub(FILE*, struct Register*, struct Register*);
struct Register* emit_and(FILE*, struct Register*, struct Register*);
struct Register* emit_or(FILE*, struct Register*, struct Register*);
struct Register* emit_cmp(FILE*, struct Register*, struct Register*);
struct Register* emit_is_zero(FILE*, struct Register*);
struct Register* emit_is_equal(FILE*, struct Register*, struct Register*);
struct Register* emit_is_not_equal(FILE*, struct Register*, struct Register*);
struct Register* emit_is_more(FILE*, struct Register*, struct Register*);
struct Register* emit_is_more_or_equal(FILE*, struct Register*, struct Register*);
struct Register* emit_is_less(FILE*, struct Register*, struct Register*);
struct Register* emit_is_less_or_equal(FILE*, struct Register*, struct Register*);
struct Register* emit_left_shift(FILE*, struct Register*, struct Register*);
struct Register* emit_right_shift(FILE*, struct Register*, struct Register*);

#endif