#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "generator.h"
#include "lexer.h"
#include "messages.h"
#include "parser.h"
#include "register.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"
#include "list.h"
#include "target.h"

int local_stack_usage = 0;

static struct Register* visit(struct Node*, FILE *fp, int);

static void visit_all(struct List* listRoot, FILE *fp, int depth) {
    if (listRoot == NULL) return;
    struct List* current_entry = listRoot;
    do {
        struct Register* reg = visit((struct Node*)current_entry->value, fp, depth);
        free_reg(reg);   // Free registers that were allocated but never used for anything :(
    } while (list_next(&current_entry));
}

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }
}

static struct Register* get_address(struct Node* node, FILE *fp, int depth) {
    struct Register* pointer_reg;

    if (node->kind == N_VARIABLE) {
        printf("%-32s", node->type->name);
        print_indent(depth);
        printf("Variable: %s\n", node->Assignment.left->token->value);

        pointer_reg = allocate_reg(2);
        if (node->Variable.symbol->global || node->Variable.symbol->is_extern) {
            fprintf(fp, "\tmov %s, %s\n", pointer_reg->name, node->Variable.symbol->token->value);
        } else {
            fprintf(fp, "\tmov %s, sp+%d ; %s\n", pointer_reg->name, get_symbol_stack_offset(node->Variable.symbol, node->scope)+local_stack_usage, node->Variable.symbol->token->value);
        }
    } else if ((node->kind == N_UNARY) && (strcmp(node->token->value, "*") == 0)) {
        printf("%-32s", node->type->name);
        print_indent(depth);
        printf("UnaryOp: *\n");

        pointer_reg = visit(node->UnaryOp.left, fp, depth+1);
    } else {
        error(node->token, "lvalue required as left operand of assignment");
    }

    return pointer_reg;
}

static void visit_program(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("Program:\n");

    // Program setup
    fprintf(fp, "#bank RAM\n\n");
    fprintf(fp, "#addr 0x8100\n\n");

    fprintf(fp, "_start:\n");

    // Initialise global variables
    visit_all(node->Program.global_variables, fp, depth+1);

    // Call main
    fprintf(fp, "\tcall main\n");
    fprintf(fp, "\tret\n\n");

    // Generate code for all functions
    visit_all(node->Program.function_declarations, fp, depth+1);

    // Label address at end of program, heap starts here
    fprintf(fp, "heap_start:\n\n");
}

static void visit_var_decl(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("Variable declaration: %s\n", node->VarDecl.symbol->token->value);
    
    if (node->VarDecl.assignment != NULL) {
        free_reg(visit(node->VarDecl.assignment, fp, depth+1));
    }

    struct Symbol* symbol = node->VarDecl.symbol;
    if (symbol->global && !symbol->is_extern) {
        // TODO this is a nasty hack, space reservations should go at end of file
        fprintf(fp, "\tjmp $+%d+3\n", node->type->size);
        fprintf(fp, "%s:\n", node->token->value);
        fprintf(fp, "\t#res %d\n", node->type->size);
    }
}

// TODO need to preserve registers
static void visit_func_decl(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("Function declaration: %s %s\n", node->type->name, node->token->value);

    fprintf(fp, "%s:\n", node->token->value);

    visit_all(node->FunctionDecl.formal_parameters, fp, depth+1);
    free_reg(visit(node->FunctionDecl.block, fp, depth+1));

    fprintf(fp, ".%s_exit:\n", node->token->value);
    fprintf(fp, "\tret\n\n");
}

static void visit_block(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("Block:\n");

    // Allocate stack space
    if (node->scope->stack_size != 0) fprintf(fp, "\tmov sp, sp-%d\n", node->scope->stack_size);

    visit_all(node->Block.statements, fp, depth+1);

    // Deallocate
    if (node->scope->stack_size != 0) fprintf(fp, "\tmov sp, sp+%d\n", node->scope->stack_size);
}

static struct Register* visit_number(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("Number: %s\n", node->token->value);

    struct Register* reg = allocate_reg(node->type->size);
    fprintf(fp, "\tmov %s, %s\n", reg->name, node->token->value);
    return reg;
}

static struct Register* visit_string(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("String: %s\n", node->token->value);

    static int label_count = 0;

    // TODO this is a nasty hack, constant data should go at end of file
    fprintf(fp, "\tjmp .string_skip_%d\n", label_count);
    fprintf(fp, ".string_%d:\n", label_count);
    fprintf(fp, "\t#d %s\n", node->token->value);
    fprintf(fp, "\t#d8 0\n");
    fprintf(fp, ".string_skip_%d:\n", label_count);

    struct Register* reg = allocate_reg(node->type->size);
    fprintf(fp, "\tmov %s, .string_%d\n", reg->name, label_count);

    label_count += 1;

    return reg;
}

static struct Register* visit_variable(struct Node* node, FILE *fp, int depth) {
    // printf("%-32s", node->type->name);
    // print_indent(depth);
    // printf("Variable: %s\n", node->token->value);

    struct Register* pointer_reg = get_address(node, fp, depth);
    struct Register* value_reg = allocate_reg(node->type->size);

    fprintf(fp, "\tmov %s, [%s]\n", value_reg->name, pointer_reg->name);

    free_reg(pointer_reg);

    return value_reg;
}

static struct Register* cast(struct Register* reg, struct Type* from_type, struct Type* to_type, FILE *fp) {
    if (from_type->kind == to_type->kind) return reg;

    printf("cast from '%s' to '%s'\n", from_type->name, to_type->name);
    
    struct Register* original_reg = reg;
    free_reg(reg);
    reg = allocate_reg(to_type->size);

    if ((from_type->kind == TY_INT) && (to_type->kind == TY_CHAR)) fprintf(fp, "\tmov %s, %s ; (%s)\n", reg->name, original_reg->high_reg->name, to_type->name);
    else if ((from_type->kind == TY_CHAR) && (to_type->kind == TY_INT)) fprintf(fp, "\tmov %s, 0 ; (%s)\n\tmov %s, %s\n", reg->high_reg->name, to_type->name, reg->low_reg->name, original_reg->name);
    else if ((from_type->kind == TY_POINTER) && (to_type->kind == TY_INT)) ;
    else if ((from_type->kind == TY_INT) && (to_type->kind == TY_POINTER)) ;
    else error(NULL, "cast from '%s' to '%s' not implemented", from_type->name, to_type->name);

    return reg;
}

static struct Register* visit_assignment(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    // printf("Assignment: %s\n", node->Assignment.left->token->value);
    printf("Assignment:\n");

    struct Register* value_reg = visit(node->Assignment.right, fp, depth+1);

    fprintf(fp, "\tpush %s\n", value_reg->name);  
    local_stack_usage += value_reg->size;
    free_reg(value_reg);

    struct Register* pointer_reg = get_address(node->Assignment.left, fp, depth+1);

    value_reg = allocate_reg(value_reg->size);
    fprintf(fp, "\tpop %s\n", value_reg->name);  
    local_stack_usage -= value_reg->size;

    value_reg = cast(value_reg, node->Assignment.right->type, node->Assignment.left->type, fp);

    fprintf(fp, "\tmov [%s], %s\n", pointer_reg->name, value_reg->name);  
    
    free_reg(pointer_reg);

    return value_reg;
}

static struct Register* visit_bin_op(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("BinOp: %s\n", node->token->value);

    struct Register* left_reg = visit(node->BinOp.left, fp, depth+1);
    left_reg = cast(left_reg, node->BinOp.left->type, node->type, fp);
    fprintf(fp, "\tpush %s\n", left_reg->name);
    free_reg(left_reg);
    local_stack_usage += left_reg->size;

    struct Register* right_reg = visit(node->BinOp.right, fp, depth+1);
    right_reg = cast(right_reg, node->BinOp.right->type, node->type, fp);
    left_reg = allocate_reg(left_reg->size);
    fprintf(fp, "\tpop %s\n", left_reg->name);
    local_stack_usage -= left_reg->size;

    if (left_reg->size != right_reg->size) error(node->token, "cannot work on registers of different sizes");

    // If right is in accumulator move it, will likely need to put left in accumulator
    if (strcmp(right_reg->name, "a") == 0) {
        right_reg = allocate_reg(1);
        fprintf(fp, "\tmov %s, a\n", right_reg->name);
        free_reg(&registers[0]);
    }

    // Perform operation
    if (node->token->kind == TK_PLUS) left_reg = emit_add(fp, left_reg, right_reg);
    else if (node->token->kind == TK_MINUS) left_reg = emit_sub(fp, left_reg, right_reg);
    else if (node->token->kind == TK_AMPERSAND) left_reg = emit_and(fp, left_reg, right_reg);
    else if (node->token->kind == TK_BAR) left_reg = emit_or(fp, left_reg, right_reg);
    else if (node->token->kind == TK_LSHIFT) left_reg = emit_left_shift(fp, left_reg, right_reg);
    else if (node->token->kind == TK_RSHIFT) left_reg = emit_right_shift(fp, left_reg, right_reg);
    else if (node->token->kind == TK_MORE) left_reg = emit_is_more(fp, left_reg, right_reg);
    else if (node->token->kind == TK_LESS) left_reg = emit_is_less(fp, left_reg, right_reg);
    else if (node->token->kind == TK_MORE_EQUAL) left_reg = emit_is_more_or_equal(fp, left_reg, right_reg);
    else if (node->token->kind == TK_LESS_EQUAL) left_reg = emit_is_less_or_equal(fp, left_reg, right_reg);
    else if (node->token->kind == TK_EQUAL) left_reg = emit_is_equal(fp, left_reg, right_reg);
    else if (node->token->kind == TK_NOT_EQUAL) left_reg = emit_is_not_equal(fp, left_reg, right_reg);
    else error(node->token, "invalid binop node");

    return left_reg;
}

static struct Register* visit_unary_op(struct Node* node, FILE *fp, int depth) {
    // Perform operation
    if (strcmp(node->token->value, "+") == 0) {
        printf("%-32s", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        // Don't need to do anything here
        
        struct Register* left_reg = visit(node->UnaryOp.left, fp, depth+1);

        return left_reg;
    } else if (strcmp(node->token->value, "-") == 0) {
        printf("%-32s", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        struct Register* left_reg = visit(node->UnaryOp.left, fp, depth+1);

        // Can only do 8-bit operations for now
        if (left_reg->size > 1) {
            error(node->token, "only 8-bit operations supported for now");
        }

        // Move left out of accumulator if necessary
        if (strcmp(left_reg->name, "a") == 0) {
            left_reg = allocate_reg(1);
            fprintf(fp, "\tmov %s, a\n", left_reg->name);
            fprintf(fp, "\tmov a, 0\n");
            fprintf(fp, "\tsub %s\n", left_reg->name);
            fprintf(fp, "\tmov a, %s\n", left_reg->name);
            free_reg(left_reg);
            left_reg = &registers[0];
        } else {
            fprintf(fp, "\tmov a, 0\n");
            fprintf(fp, "\tsub %s\n", left_reg->name);
        }

        return left_reg;
    } else if (strcmp(node->token->value, "*") == 0) {
        struct Register* pointer_reg = get_address(node, fp, depth);
        free_reg(pointer_reg);
        struct Register* value_reg = allocate_reg(node->UnaryOp.left->Variable.symbol->type->base->size);

        fprintf(fp, "\tmov %s, [%s]\n", value_reg->name, pointer_reg->name);
        
        return value_reg;
    } else if (strcmp(node->token->value, "&") == 0) {
        printf("%-32s", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        struct Register* pointer_reg = get_address(node->UnaryOp.left, fp, depth+1);

        return pointer_reg;
    } else if (node->token->kind == TK_INC) {
        printf("%-32s", node->type->name);
        print_indent(depth);
        printf("UnaryOp: ++\n");

        struct Register* left_reg = visit(node->UnaryOp.left, fp, depth+1);
        fprintf(fp, "\tinc %s\n", left_reg->name);

        return left_reg;
    } else if (node->token->kind == TK_DEC) {
        printf("%-32s", node->type->name);
        print_indent(depth);
        printf("UnaryOp: --\n");

        struct Register* left_reg = visit(node->UnaryOp.left, fp, depth+1);
        fprintf(fp, "\tdec %s\n", left_reg->name);

        return left_reg;
    } else {
        error(node->token, "invalid unary operator");
    }
}

static void visit_return(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("Return:\n");

    // Get return value
    // TODO can't return nothing lol
    struct Register* reg = visit(node->Return.expr, fp, depth+1);

    // TODO return in a 16-bit register or on the stack
    if (reg->size > 1) error(node->token, "only 8-bit return supported for now");

    // Move return value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", reg->name);

    // TODO this needs account for returns from inside futher scopes
    // Restore stack
    if (node->scope->stack_size != 0) fprintf(fp, "\tmov sp, sp+%d\n", node->scope->stack_size);

    fprintf(fp, "\tret\n");
    free_reg(reg);
}

static void visit_if(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("If:\n");

    static int label_count = 0;
    int tmp_label_count = label_count;
    label_count++;

    // Get test value
    struct Register* reg = visit(node->Return.expr, fp, depth+1);

    // Push accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        fprintf(fp, "\tpush a\n");
        local_stack_usage += 1;
    }

    // Move test value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", reg->name);

    // Compare
    fprintf(fp, "\tcmp 0\n");

    // Restore accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        fprintf(fp, "\tpop a\n");
        local_stack_usage -= 1;
    }

    // Jump if equal 0
    fprintf(fp, "\tje .if_false_%d\n", tmp_label_count);
    free_reg(reg);

    // Visit true branch
    fprintf(fp, ".if_true_%d:\n", tmp_label_count);
    free_reg(visit(node->If.true_statement, fp, depth+1));
    fprintf(fp, "\tjmp .if_exit_%d\n", tmp_label_count);

    // Visit false branch
    fprintf(fp, ".if_false_%d:\n", tmp_label_count);
    if (node->If.false_statement != NULL) free_reg(visit(node->If.false_statement, fp, depth+1));

    fprintf(fp, ".if_exit_%d:\n", tmp_label_count);
}

static void visit_while(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("While:\n");

    static int label_count = 0;
    int tmp_label_count = label_count;
    label_count++;

    fprintf(fp, ".while_start_%d:\n", tmp_label_count);

    // Get test value
    struct Register* reg = visit(node->Return.expr, fp, depth+1);

    // Push accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        fprintf(fp, "\tpush a\n");
        local_stack_usage += 1;
    }

    // Move test value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", reg->name);

    // Compare
    fprintf(fp, "\tcmp 0\n");

    // Restore accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        fprintf(fp, "\tpop a\n");
        local_stack_usage -= 1;
    }

    // Jump if equal 0
    fprintf(fp, "\tje .while_exit_%d\n", tmp_label_count);
    free_reg(reg);

    // Visit loop statement
    fprintf(fp, ".while_contents_%d:\n", tmp_label_count);
    free_reg(visit(node->While.loop_statement, fp, depth+1));

    // Go back to start of loop
    fprintf(fp, "\tjmp .while_start_%d\n", tmp_label_count);

    fprintf(fp, ".while_exit_%d:\n", tmp_label_count);

    tmp_label_count++;
}

static struct Register* visit_func_call(struct Node* node, FILE *fp, int depth) {
    printf("%-32s", node->type->name);
    print_indent(depth);
    printf("Call: %s\n", node->token->value);

    struct Symbol* symbol = node->FuncCall.symbol;

    // Push accumulator if necessary
    int preserve_a = !registers[0].free;
    if (preserve_a) {
        emit_push(fp, &registers[0]);
        local_stack_usage += registers[0].size;
    }

    // Push parameters
    int func_stack_usage = 0;
    if (node->FuncCall.parameters != NULL) {
        struct List* current_entry = node->FuncCall.parameters;
        do {
            struct Node* actual_param = (struct Node*)current_entry->value;
            
            struct Register* reg = visit(actual_param, fp, depth+1);
            // reg = cast(reg, actual_param->node->type, formal_param->type, fp);
            emit_push(fp, reg);
            func_stack_usage += reg->size;
            free_reg(reg);
        } while (list_next(&current_entry));
    }
    
    fprintf(fp, "\tcall %s\n", node->token->value);
    fprintf(fp, "\tmov sp, sp+%d\n", func_stack_usage);

    // Move result out of accumulator if necessary
    struct Register* result_reg = allocate_reg(1);
    if (preserve_a) emit_move(fp, result_reg, &registers[0]);

    // Restore accumulator if necessary
    if (preserve_a) {
        emit_pop(fp, &registers[0]);
        local_stack_usage -= registers[0].size;
    }

    return result_reg;
}

static struct Register* visit(struct Node* node, FILE *fp, int depth) {
    switch (node->kind) {
        case N_PROGRAM:
            visit_program(node, fp, depth);
            return NULL;
        case N_VAR_DECL:
            visit_var_decl(node, fp, depth);
            return NULL;
        case N_FUNC_DECL:
            visit_func_decl(node, fp, depth);
            return NULL;
        case N_BLOCK:
            visit_block(node, fp, depth);
            return NULL;
        case N_NUMBER:
            return visit_number(node, fp, depth);
        case N_STRING:
            return visit_string(node, fp, depth);
        case N_VARIABLE:
            return visit_variable(node, fp, depth);
        case N_ASSIGNMENT:
            return visit_assignment(node, fp, depth);
        case N_BINOP:
            return visit_bin_op(node, fp, depth);
        case N_UNARY:
            return visit_unary_op(node, fp, depth);
        case N_RETURN:
            visit_return(node, fp, depth);
            return NULL;
        case N_IF:
            visit_if(node, fp, depth);
            return NULL;
        case N_WHILE:
            visit_while(node, fp, depth);
            return NULL;
        case N_FUNC_CALL:
            return visit_func_call(node, fp, depth);
    }

    error(node->token, "invalid node kind");
    return NULL;
}

void generate(struct Node* root_node, char* filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) error(NULL, "unable to create output file '%s'", filename);

    visit(root_node, fp, 0);

    fclose(fp);
}