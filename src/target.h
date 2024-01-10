#ifndef _TARGET_H
#define _TARGET_H

struct _IO_FILE;
typedef struct _IO_FILE FILE;

struct Register;

void emit_move(FILE*, struct Register*, struct Register*);
void emit_push(FILE*, struct Register*);
void emit_pop(FILE*, struct Register*);

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