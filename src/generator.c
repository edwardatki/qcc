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

int local_stack_usage = 0;

static struct Register registers[] = { {.name="a", .size=1, .free=1, .high_reg=NULL, .low_reg=NULL},
                                    {.name="b", .size=1, .free=1, .high_reg=NULL, .low_reg=NULL},
                                    {.name="c", .size=1, .free=1, .high_reg=NULL, .low_reg=NULL},
                                    {.name="d", .size=1, .free=1, .high_reg=NULL, .low_reg=NULL},
                                    {.name="e", .size=1, .free=1, .high_reg=NULL, .low_reg=NULL},
                                    {.name="bc", .size=2, .free=1, .high_reg=&registers[1], .low_reg=&registers[2]},
                                    {.name="de", .size=2, .free=1, .high_reg=&registers[3], .low_reg=&registers[4]}};

static struct Register* allocate_reg(int size) {
    for (int i = 0; i < sizeof(registers)/sizeof(struct Register); i++) {
        // if (i == 0) continue; // TEMPORARY don't allocate a so we never have to push it to the stack
        struct Register* reg = &registers[i];
        if ((reg->size >= size) && reg->free) {
            // If sub registers are in use then can't allocate
            if ((reg->high_reg != NULL) && !reg->high_reg->free) continue;
            if ((reg->low_reg != NULL) && !reg->low_reg->free) continue;

            // Mark register as used
            reg->free = 0;

            // Mark sub registers as used too
            if (reg->high_reg != NULL) reg->high_reg->free = 0;
            if (reg->low_reg != NULL) reg->low_reg->free = 0;

            // printf("ALLOCATED REGISTER %s\n", reg->name);
            return reg;
        }
    }

    printf("a: %s\n", registers[0].free ? GRN "free" RESET : RED "used" RESET);
    printf("b: %s\n", registers[1].free ? GRN "free" RESET : RED "used" RESET);
    printf("c: %s\n", registers[2].free ? GRN "free" RESET : RED "used" RESET);
    printf("d: %s\n", registers[3].free ? GRN "free" RESET : RED "used" RESET);
    printf("e: %s\n", registers[4].free ? GRN "free" RESET : RED "used" RESET);
    printf("bc: %s\n", registers[5].free ? GRN "free" RESET : RED "used" RESET);
    printf("de: %s\n", registers[6].free ? GRN "free" RESET : RED "used" RESET);
    error(NULL, "unable to allocate register of size %d", size);
}

static void free_reg(struct Register* reg) {
    if (reg == NULL) return;

    // Mark register as free
    reg->free = 1;

    // Mark sub registers as free too
    if (reg->high_reg != NULL) reg->high_reg->free = 1;
    if (reg->low_reg != NULL) reg->low_reg->free = 1;

    // printf("FREED REGISTER %s\n", reg->name);
}

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
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("Variable: %s\n", node->Assignment.left->token->value);

        pointer_reg = allocate_reg(2);
        if (node->Variable.symbol->global) {
            fprintf(fp, "\tmov %s, %s\n", pointer_reg->name, node->Variable.symbol->token->value);
        } else {
            fprintf(fp, "\tmov %s, sp+%d ; %s\n", pointer_reg->name, get_symbol_stack_offset(node->Variable.symbol, node->scope)+local_stack_usage, node->Variable.symbol->token->value);
        }
    } else if ((node->kind == N_UNARY) && (strcmp(node->token->value, "*") == 0)) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: *\n");

        pointer_reg = visit(node->UnaryOp.left, fp, depth+1);
    } else {
        error(node->token, "lvalue required as left operand of assignment");
    }

    return pointer_reg;
}

static void visit_program(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Program:\n");

    // Program setup
    fprintf(fp, "#include \"architecture.asm\"\n\n");
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
}

static void visit_var_decl(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Variable declaration: %s\n", node->VarDecl.symbol->token->value);
    
    if (node->VarDecl.assignment != NULL) {
        free_reg(visit(node->VarDecl.assignment, fp, depth+1));
    }

    struct Symbol* symbol = node->VarDecl.symbol;
    if (symbol->global) {
        // TODO this is a nasty hack, space reservations should go at end of file
        fprintf(fp, "\tjmp $+%d+3\n", node->type->size);
        fprintf(fp, "%s:\n", node->token->value);
        fprintf(fp, "\t#res %d\n", node->type->size);
    }
}

// TODO need to preserve registers
static void visit_func_decl(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Function declaration: %s %s\n", node->type->name, node->token->value);

    fprintf(fp, "%s:\n", node->token->value);

    visit_all(node->FunctionDecl.formal_parameters, fp, depth+1);
    free_reg(visit(node->FunctionDecl.block, fp, depth+1));

    fprintf(fp, ".%s_exit:\n", node->token->value);
    fprintf(fp, "\tret\n\n");
}

static void visit_block(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Block:\n");

    // Allocate stack space
    if (node->scope->stack_size != 0) fprintf(fp, "\tmov sp, sp-%d\n", node->scope->stack_size);

    visit_all(node->Block.statements, fp, depth+1);

    // Deallocate
    if (node->scope->stack_size != 0) fprintf(fp, "\tmov sp, sp+%d\n", node->scope->stack_size);
}

static struct Register* visit_number(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Number: %s\n", node->token->value);

    struct Register* reg = allocate_reg(node->type->size);
    fprintf(fp, "\tmov %s, %s\n", reg->name, node->token->value);
    return reg;
}

static struct Register* visit_string(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("String: %s\n", node->token->value);

    static int label_count = 0;

    // TODO this is a nasty hack, constant data should go at end of file
    fprintf(fp, "\tjmp $+%d+3\n", (int)strlen(node->token->value)-1);
    fprintf(fp, "string_%d:\n", label_count);
    fprintf(fp, "\t#d %s\n", node->token->value);
    fprintf(fp, "\t#d8 0\n");

    struct Register* reg = allocate_reg(node->type->size);
    fprintf(fp, "\tmov %s, string_%d\n", reg->name, label_count);

    label_count += 1;

    return reg;
}

static struct Register* visit_variable(struct Node* node, FILE *fp, int depth) {
    // printf("%s\t", node->type->name);
    // print_indent(depth);
    // printf("Variable: %s\n", node->token->value);

    struct Register* pointer_reg = get_address(node, fp, depth);
    struct Register* value_reg = allocate_reg(node->type->size);

    fprintf(fp, "\tmov %s, [%s]\n", value_reg->name, pointer_reg->name);

    free_reg(pointer_reg);

    return value_reg;
}

static struct Register* cast(struct Register* reg, struct Type* from_type, struct Type* to_type, FILE *fp) {
    struct Register* original_reg = reg;
    free_reg(reg);
    reg = allocate_reg(to_type->size);

    if (from_type->kind == to_type->kind) return reg;

    if ((from_type->kind == TY_INT) && (to_type->kind == TY_CHAR)) fprintf(fp, "\tmov %s, %s ; (%s)\n", reg->name, original_reg->high_reg->name, to_type->name);
    else if ((from_type->kind == TY_CHAR) && (to_type->kind == TY_INT)) fprintf(fp, "\tmov %s, 0 ; (%s)\n\tmov %s, %s\n", reg->high_reg->name, to_type->name, reg->low_reg->name, original_reg->name);
    else if ((from_type->kind == TY_POINTER) && (to_type->kind == TY_INT)) ;
    else if ((from_type->kind == TY_INT) && (to_type->kind == TY_POINTER)) ;
    else error(NULL, "cast from '%s' to '%s' not implemented", from_type->name, to_type->name);

    return reg;
}

static struct Register* visit_assignment(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    // printf("Assignment: %s\n", node->Assignment.left->token->value);
    printf("Assignment:\n");

    struct Register* value_reg = visit(node->Assignment.right, fp, depth+1);
    struct Register* pointer_reg = get_address(node->Assignment.left, fp, depth+1);

    value_reg = cast(value_reg, node->Assignment.right->type, node->Assignment.left->type, fp);

    fprintf(fp, "\tmov [%s], %s\n", pointer_reg->name, value_reg->name);  
    
    free_reg(pointer_reg);

    return value_reg;
}

static struct Register* visit_bin_op(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("BinOp: %s\n", node->token->value);

    struct Register* left_reg = visit(node->BinOp.left, fp, depth+1);
    struct Register* right_reg = visit(node->BinOp.right, fp, depth+1);

    // Can only do 8-bit operations for now
    if ((left_reg->size > 1) | (right_reg->size > 1)) {
        error(node->token, "only 8-bit operations supported for now");
    }

    // Push accumulator if necessary
    if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) {
        fprintf(fp, "\tpush a\n");
        local_stack_usage += 1;
    }

    // Move left into accumulator if necessary
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov a, %s\n", left_reg->name);

    // Perform operation
    if (strcmp(node->token->value, "+") == 0) {
        fprintf(fp, "\tadd %s\n", right_reg->name);
    } else if (strcmp(node->token->value, "-") == 0) {
        fprintf(fp, "\tsub %s\n", right_reg->name);
    } else if (strcmp(node->token->value, "&") == 0) {
        fprintf(fp, "\tand %s\n", right_reg->name);
    } else if (strcmp(node->token->value, "|") == 0) {
        fprintf(fp, "\tor %s\n", right_reg->name);
    } else if (strcmp(node->token->value, "<<") == 0) { // TODO this is a rotate instead of shift :(
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
    } else if (strcmp(node->token->value, ">>") == 0) { // TODO this is a rotate instead of shift :(
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
    } else if (strcmp(node->token->value, ">") == 0) {
        static int label_count = 0;
        
        fprintf(fp, "\tcmp %s\n", right_reg->name);
        fprintf(fp, "\tjnc .cmp_more_false_%d\n", label_count); // A <= right

        fprintf(fp, ".cmp_more_true_%d:\n", label_count);
        fprintf(fp, "\tmov a, 1\n");
        fprintf(fp, "\tjmp .cmp_more_exit_%d\n", label_count);

        fprintf(fp, ".cmp_more_false_%d:\n", label_count);
        fprintf(fp, "\tmov a, 0\n");

        fprintf(fp, ".cmp_more_exit_%d:\n", label_count);

        label_count++;
    } else if (strcmp(node->token->value, "<") == 0) {
        static int label_count = 0;
        
        fprintf(fp, "\tcmp %s\n", right_reg->name);
        fprintf(fp, "\tjc .cmp_less_false_%d\n", label_count);  // A > right
        fprintf(fp, "\tje .cmp_less_false_%d\n", label_count);  // A == right

        fprintf(fp, ".cmp_less_true_%d:\n", label_count);
        fprintf(fp, "\tmov a, 1\n");
        fprintf(fp, "\tjmp .cmp_less_exit_%d\n", label_count);

        fprintf(fp, ".cmp_less_false_%d:\n", label_count);
        fprintf(fp, "\tmov a, 0\n");

        fprintf(fp, ".cmp_less_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, ">=") == 0) {
        static int label_count = 0;
        
        fprintf(fp, "\tcmp %s\n", right_reg->name);
        fprintf(fp, "\tjne .cmp_more_equal_true_%d\n", label_count); // A == right
        fprintf(fp, "\tjnc .cmp_more_equal_false_%d\n", label_count); // A <= right

        fprintf(fp, ".cmp_more_equal_true_%d:\n", label_count);
        fprintf(fp, "\tmov a, 1\n");
        fprintf(fp, "\tjmp .cmp_more_exit_%d\n", label_count);

        fprintf(fp, ".cmp_more_equal_false_%d:\n", label_count);
        fprintf(fp, "\tmov a, 0\n");

        fprintf(fp, ".cmp_more_equal_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, "<=") == 0) {
        static int label_count = 0;
        
        fprintf(fp, "\tcmp %s\n", right_reg->name);
        fprintf(fp, "\tjc .cmp_less_equal_false_%d\n", label_count); // A > right

        fprintf(fp, ".cmp_less_equal_true_%d:\n", label_count);
        fprintf(fp, "\tmov a, 1\n");
        fprintf(fp, "\tjmp .cmp_less_equal_exit_%d\n", label_count);

        fprintf(fp, ".cmp_less_equal_false_%d:\n", label_count);
        fprintf(fp, "\tmov a, 0\n");

        fprintf(fp, ".cmp_less_equal_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, "==") == 0) {
        static int label_count = 0;
        
        fprintf(fp, "\tcmp %s\n", right_reg->name);
        fprintf(fp, "\tjne .cmp_equal_false_%d\n", label_count); // A != right

        fprintf(fp, ".cmp_equal_true_%d:\n", label_count);
        fprintf(fp, "\tmov a, 1\n");
        fprintf(fp, "\tjmp .cmp_equal_exit_%d\n", label_count);

        fprintf(fp, ".cmp_equal_false_%d:\n", label_count);
        fprintf(fp, "\tmov a, 0\n");

        fprintf(fp, ".cmp_equal_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, "!=") == 0) {
        static int label_count = 0;
        
        fprintf(fp, "\tcmp %s\n", right_reg->name);
        fprintf(fp, "\tje .cmp_not_equal_false_%d\n", label_count); // A != right

        fprintf(fp, ".cmp_not_equal_true_%d:\n", label_count);
        fprintf(fp, "\tmov a, 1\n");
        fprintf(fp, "\tjmp .cmp_not_equal_exit_%d\n", label_count);

        fprintf(fp, ".cmp_not_equal_false_%d:\n", label_count);
        fprintf(fp, "\tmov a, 0\n");

        fprintf(fp, ".cmp_not_equal_exit_%d:\n", label_count);
    } else {
        fprintf(fp, "\t???\n");
    }
    
    free_reg(right_reg);

    // Move result from accumulator back to left if necessary
    if (strcmp(left_reg->name, "a") != 0) fprintf(fp, "\tmov %s, a\n", left_reg->name);

    // Restore accumulator if necessary
    if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) {
        fprintf(fp, "\tpop a\n");
        local_stack_usage -= 1;
    }

    return left_reg;
}

static struct Register* visit_unary_op(struct Node* node, FILE *fp, int depth) {
    // Perform operation
    if (strcmp(node->token->value, "+") == 0) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        // Don't need to do anything here
        struct Register* left_reg = visit(node->UnaryOp.left, fp, depth+1);

        return left_reg;
    } else if (strcmp(node->token->value, "-") == 0) {
        printf("%s\t", node->type->name);
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
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        struct Register* pointer_reg = get_address(node->UnaryOp.left, fp, depth+1);

        return pointer_reg;
    } else if (strcmp(node->token->value, "++") == 0) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        struct Register* left_reg = visit(node->UnaryOp.left, fp, depth+1);
        fprintf(fp, "\tinc %s\n", left_reg->name);

        return left_reg;
    } else if (strcmp(node->token->value, "--") == 0) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        struct Register* left_reg = visit(node->UnaryOp.left, fp, depth+1);
        fprintf(fp, "\tdec %s\n", left_reg->name);

        return left_reg;
    } else {
        error(node->token, "invalid unary operator");
    }
}

static void visit_return(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Return:\n");

    // Get return value
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
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("If:\n");

    static int label_count = 0;

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
    fprintf(fp, "\tje .if_false_%d\n", label_count);
    free_reg(reg);

    // Visit true branch
    fprintf(fp, ".if_true_%d:\n", label_count);
    free_reg(visit(node->If.true_statement, fp, depth+1));
    fprintf(fp, "\tjmp .if_exit_%d\n", label_count);

    // Visit false branch
    fprintf(fp, ".if_false_%d:\n", label_count);
    if (node->If.false_statement != NULL) free_reg(visit(node->If.false_statement, fp, depth+1));

    fprintf(fp, ".if_exit_%d:\n", label_count);

    label_count++;
}

static void visit_while(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("While:\n");

    static int label_count = 0;

    fprintf(fp, ".while_start_%d:\n", label_count);

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
    fprintf(fp, "\tje .while_exit_%d\n", label_count);
    free_reg(reg);

    // Visit loop statement
    fprintf(fp, ".while_contents_%d:\n", label_count);
    free_reg(visit(node->While.loop_statement, fp, depth+1));

    // Go back to start of loop
    fprintf(fp, "\tjmp .while_start_%d\n", label_count);

    fprintf(fp, ".while_exit_%d:\n", label_count);

    label_count++;
}

static struct Register* visit_func_call(struct Node* node, FILE *fp, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Call: %s\n", node->token->value);

    struct Symbol* symbol = node->FuncCall.symbol;

    // Push accumulator if necessary
    int preserve_a = !registers[0].free;
    if (preserve_a) {
        fprintf(fp, "\tpush a\n");
        local_stack_usage += 1;
    }

    // Push parameters
    int func_stack_usage = 0;
    if (node->FuncCall.parameters != NULL) {
        struct List* current_entry = node->FuncCall.parameters;
        do {
            struct Node* actual_param = (struct Node*)current_entry->value;
            
            struct Register* reg = visit(actual_param, fp, depth+1);
            // reg = cast(reg, actual_param->node->type, formal_param->type, fp);
            fprintf(fp, "\tpush %s\n", reg->name);
            func_stack_usage += reg->size;
            free_reg(reg);
        } while (list_next(&current_entry));
    }
    
    fprintf(fp, "\tcall %s\n", node->token->value);
    fprintf(fp, "\tmov sp, sp+%d\n", func_stack_usage);

    // Move result out of accumulator if necessary
    struct Register* result_reg = allocate_reg(1);
    if (preserve_a) fprintf(fp, "\tmov %s, a\n", result_reg->name);

    // Restore accumulator if necessary
    if (preserve_a) {
        fprintf(fp, "\tpop a\n");
        local_stack_usage -= 1;
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
    if (!fp) error(NULL, "unable to open file '%s'", filename);

    visit(root_node, fp, 0);

    fclose(fp);
}