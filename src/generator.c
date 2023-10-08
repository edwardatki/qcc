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

static void visit_var_decl(struct Node*, int);
static void visit_func_decl(struct Node*, int);
static void visit_block(struct Node*, int);
static struct Register* visit_number(struct Node*, int);
static struct Register* visit_variable(struct Node*, int);
static void visit_assignment(struct Node*, int);
static struct Register* visit_bin_op(struct Node*, int);
static void visit_return(struct Node*, int);
static struct Register* visit(struct Node*, int);

static char out_buffer [8192]; // TODO just write directly to file pointer
static char* out_pointer = out_buffer;

int local_stack_usage = 0;

static struct Register registers[] = { {.name="a", .size=1, .free=1, .sub_reg={NULL, NULL}},
                                    {.name="b", .size=1, .free=1, .sub_reg={NULL, NULL}},
                                    {.name="c", .size=1, .free=1, .sub_reg={NULL, NULL}},
                                    {.name="d", .size=1, .free=1, .sub_reg={NULL, NULL}},
                                    {.name="e", .size=1, .free=1, .sub_reg={NULL, NULL}},
                                    {.name="bc", .size=2, .free=1, .sub_reg={&registers[1], &registers[2]}},
                                    {.name="de", .size=2, .free=1, .sub_reg={&registers[3], &registers[4]}}};

static struct Register* allocate_reg(int size) {
    for (int i = 0; i < sizeof(registers)/sizeof(struct Register); i++) {
        // if (i == 0) continue; // TEMPORARY don't allocate a so we never have to push it to the stack
        struct Register* reg = &registers[i];
        if ((reg->size >= size) && reg->free) {
            // If sub registers are in use then can't allocate
            if ((reg->sub_reg[0] != NULL) && !reg->sub_reg[0]->free) continue;
            if ((reg->sub_reg[1] != NULL) && !reg->sub_reg[1]->free) continue;

            // Mark register as used
            reg->free = 0;

            // Mark sub registers as used too
            if (reg->sub_reg[0] != NULL) reg->sub_reg[0]->free = 0;
            if (reg->sub_reg[1] != NULL) reg->sub_reg[1]->free = 0;

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
    if (reg->sub_reg[0] != NULL) reg->sub_reg[0]->free = 1;
    if (reg->sub_reg[1] != NULL) reg->sub_reg[1]->free = 1;

    // printf("FREED REGISTER %s\n", reg->name);
}

static void visit_all(struct NodeListEntry* listRoot, int depth) {
    if (listRoot == NULL) return;
    struct NodeListEntry* current_entry = listRoot;
    while (current_entry->next != NULL) {
        current_entry = current_entry->next;
        struct Register* reg = visit(current_entry->node, depth);
        free_reg(reg);   // Free registers that were allocated but never used for anything :(
    }
}

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }
}

static struct Register* get_address(struct Node* node, int depth) {
    struct Register* pointer_reg;

    if (node->kind == N_VARIABLE) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("Variable: %s\n", node->Assignment.left->token->value);

        pointer_reg = allocate_reg(2);
        if (node->Variable.symbol->global) {
            out_pointer += sprintf(out_pointer, "\tmov %s, %s\n", pointer_reg->name, node->Variable.symbol->token->value);
        } else {
            out_pointer += sprintf(out_pointer, "\tmov %s, sp+%d ; %s\n", pointer_reg->name, get_symbol_stack_offset(node->Variable.symbol, node->scope)+local_stack_usage, node->Variable.symbol->token->value);
        }
    } else if ((node->kind == N_UNARY) && (strcmp(node->token->value, "*") == 0)) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: *\n");

        pointer_reg = visit(node->UnaryOp.left, depth+1);
    } else {
        error(node->token, "lvalue required as left operand of assignment");
    }

    return pointer_reg;
}

static void visit_program(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Program\n");

    visit_all(node->Program.function_declarations, depth+1);
}

static void visit_var_decl(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Variable declaration: %s %s\n", node->VarDecl.symbol->type->name, node->VarDecl.symbol->token->value);
    
    struct Symbol* symbol = node->VarDecl.symbol;

    // Will need to reserve space for global variables and track local variable's positions on the stack
    if (symbol->global) printf("FUCK\n");
}

// TODO need to preserve registers
static void visit_func_decl(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Function declaration: %s %s\n", node->type->name, node->token->value);

    out_pointer += sprintf(out_pointer, "%s:\n", node->token->value);

    visit_all(node->FunctionDecl.formal_parameters, depth+1);
    visit(node->FunctionDecl.block, depth+1);

    out_pointer += sprintf(out_pointer, ".%s_exit:\n", node->token->value);
    out_pointer += sprintf(out_pointer, "\tret\n\n");
}

static void visit_block(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Block\n");

    // Allocate stack space
    if (node->scope->stack_size != 0) out_pointer += sprintf(out_pointer, "\tmov sp, sp-%d\n", node->scope->stack_size);

    visit_all(node->Block.variable_declarations, depth+1);
    visit_all(node->Block.statements, depth+1);

    // Deallocate
    if (node->scope->stack_size != 0) out_pointer += sprintf(out_pointer, "\tmov sp, sp+%d\n", node->scope->stack_size);
}

static struct Register* visit_number(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Number: %s\n", node->token->value);

    struct Register* reg = allocate_reg(node->type->size);
    out_pointer += sprintf(out_pointer, "\tmov %s, %s\n", reg->name, node->token->value);
    return reg;
}

static struct Register* visit_variable(struct Node* node, int depth) {
    // printf("%s\t", node->type->name);
    // print_indent(depth);
    // printf("Variable: %s\n", node->token->value);

    struct Register* pointer_reg = get_address(node, depth);
    struct Register* value_reg = allocate_reg(node->type->size);

    out_pointer += sprintf(out_pointer, "\tmov %s, [%s]\n", value_reg->name, pointer_reg->name);

    free_reg(pointer_reg);

    return value_reg;
}

static struct Register* cast(struct Register* reg, struct Type* from_type, struct Type* to_type) {
    struct Register* original_reg = reg;
    free_reg(reg);
    reg = allocate_reg(to_type->size);

    if ((from_type->kind == TY_INT) && (to_type->kind == TY_CHAR)) out_pointer += sprintf(out_pointer, "\tmov %s, %s ; (%s)\n", reg->name, original_reg->sub_reg[1]->name, to_type->name);
    if ((from_type->kind == TY_CHAR) && (to_type->kind == TY_INT)) out_pointer += sprintf(out_pointer, "\tmov %s, 0 ; (%s)\n\tmov %s, %s\n", reg->sub_reg[0]->name, to_type->name, reg->sub_reg[1]->name, original_reg->name);

    return reg;
}

static void visit_assignment(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    // printf("Assignment: %s\n", node->Assignment.left->token->value);
    printf("Assignment:\n");

    struct Register* value_reg = visit(node->Assignment.right, depth+1);
    struct Register* pointer_reg = get_address(node->Assignment.left, depth+1);

    value_reg = cast(value_reg, node->Assignment.right->type, node->Assignment.left->type);

    out_pointer += sprintf(out_pointer, "\tmov [%s], %s\n", pointer_reg->name, value_reg->name);  
    
    free_reg(pointer_reg);
    free_reg(value_reg);
}

static struct Register* visit_bin_op(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("BinOp: %s\n", node->token->value);

    struct Register* left_reg = visit(node->BinOp.left, depth+1);
    struct Register* right_reg = visit(node->BinOp.right, depth+1);

    // Can only do 8-bit operations for now
    if ((left_reg->size > 1) | (right_reg->size > 1)) {
        error(node->token, "only 8-bit operations supported for now");
    }

    // Push accumulator if necessary
    if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) {
        out_pointer += sprintf(out_pointer, "\tpush a\n");
        local_stack_usage += 1;
    }

    // Move left into accumulator if necessary
    if (strcmp(left_reg->name, "a") != 0) out_pointer += sprintf(out_pointer, "\tmov a, %s\n", left_reg->name);

    // Perform operation
    if (strcmp(node->token->value, "+") == 0) {
        out_pointer += sprintf(out_pointer, "\tadd %s\n", right_reg->name);
    } else if (strcmp(node->token->value, "-") == 0) {
        out_pointer += sprintf(out_pointer, "\tsub %s\n", right_reg->name);
    } else if (strcmp(node->token->value, ">") == 0) {
        static int label_count = 0;
        
        out_pointer += sprintf(out_pointer, "\tcmp %s\n", right_reg->name);
        out_pointer += sprintf(out_pointer, "\tjnc .cmp_more_false_%d\n", label_count); // A <= right

        out_pointer += sprintf(out_pointer, ".cmp_more_true_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 1\n");
        out_pointer += sprintf(out_pointer, "\tjmp .cmp_more_exit_%d\n", label_count);

        out_pointer += sprintf(out_pointer, ".cmp_more_false_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 0\n");

        out_pointer += sprintf(out_pointer, ".cmp_more_exit_%d:\n", label_count);

        label_count++;
    } else if (strcmp(node->token->value, "<") == 0) {
        static int label_count = 0;
        
        out_pointer += sprintf(out_pointer, "\tcmp %s\n", right_reg->name);
        out_pointer += sprintf(out_pointer, "\tjc .cmp_less_false_%d\n", label_count);  // A > right
        out_pointer += sprintf(out_pointer, "\tje .cmp_less_false_%d\n", label_count);  // A == right

        out_pointer += sprintf(out_pointer, ".cmp_less_true_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 1\n");
        out_pointer += sprintf(out_pointer, "\tjmp .cmp_less_exit_%d\n", label_count);

        out_pointer += sprintf(out_pointer, ".cmp_less_false_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 0\n");

        out_pointer += sprintf(out_pointer, ".cmp_less_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, ">=") == 0) {
        static int label_count = 0;
        
        out_pointer += sprintf(out_pointer, "\tcmp %s\n", right_reg->name);
        out_pointer += sprintf(out_pointer, "\tjne .cmp_more_equal_true_%d\n", label_count); // A == right
        out_pointer += sprintf(out_pointer, "\tjnc .cmp_more_equal_false_%d\n", label_count); // A <= right

        out_pointer += sprintf(out_pointer, ".cmp_more_equal_true_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 1\n");
        out_pointer += sprintf(out_pointer, "\tjmp .cmp_more_exit_%d\n", label_count);

        out_pointer += sprintf(out_pointer, ".cmp_more_equal_false_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 0\n");

        out_pointer += sprintf(out_pointer, ".cmp_more_equal_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, "<=") == 0) {
        static int label_count = 0;
        
        out_pointer += sprintf(out_pointer, "\tcmp %s\n", right_reg->name);
        out_pointer += sprintf(out_pointer, "\tjc .cmp_less_equal_false_%d\n", label_count); // A > right

        out_pointer += sprintf(out_pointer, ".cmp_less_equal_true_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 1\n");
        out_pointer += sprintf(out_pointer, "\tjmp .cmp_less_equal_exit_%d\n", label_count);

        out_pointer += sprintf(out_pointer, ".cmp_less_equal_false_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 0\n");

        out_pointer += sprintf(out_pointer, ".cmp_less_equal_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, "==") == 0) {
        static int label_count = 0;
        
        out_pointer += sprintf(out_pointer, "\tcmp %s\n", right_reg->name);
        out_pointer += sprintf(out_pointer, "\tjne .cmp_equal_false_%d\n", label_count); // A != right

        out_pointer += sprintf(out_pointer, ".cmp_equal_true_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 1\n");
        out_pointer += sprintf(out_pointer, "\tjmp .cmp_equal_exit_%d\n", label_count);

        out_pointer += sprintf(out_pointer, ".cmp_equal_false_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 0\n");

        out_pointer += sprintf(out_pointer, ".cmp_equal_exit_%d:\n", label_count);
    } else if (strcmp(node->token->value, "!=") == 0) {
        static int label_count = 0;
        
        out_pointer += sprintf(out_pointer, "\tcmp %s\n", right_reg->name);
        out_pointer += sprintf(out_pointer, "\tje .cmp_not_equal_false_%d\n", label_count); // A != right

        out_pointer += sprintf(out_pointer, ".cmp_not_equal_true_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 1\n");
        out_pointer += sprintf(out_pointer, "\tjmp .cmp_not_equal_exit_%d\n", label_count);

        out_pointer += sprintf(out_pointer, ".cmp_not_equal_false_%d:\n", label_count);
        out_pointer += sprintf(out_pointer, "\tmov a, 0\n");

        out_pointer += sprintf(out_pointer, ".cmp_not_equal_exit_%d:\n", label_count);
    } else {
        out_pointer += sprintf(out_pointer, "\t???\n");
    }
    
    free_reg(right_reg);

    // Move result from accumulator back to left if necessary
    if (strcmp(left_reg->name, "a") != 0) out_pointer += sprintf(out_pointer, "\tmov %s, a\n", left_reg->name);

    // Restore accumulator if necessary
    if ((strcmp(left_reg->name, "a") != 0) && (!registers[0].free)) {
        out_pointer += sprintf(out_pointer, "\tpop a\n");
        local_stack_usage -= 1;
    }

    return left_reg;
}

static struct Register* visit_unary_op(struct Node* node, int depth) {
    // Perform operation
    if (strcmp(node->token->value, "+") == 0) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        // Don't need to do anything here
        struct Register* left_reg = visit(node->UnaryOp.left, depth+1);
        return left_reg;
    } else if (strcmp(node->token->value, "-") == 0) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        struct Register* left_reg = visit(node->UnaryOp.left, depth+1);

        // Can only do 8-bit operations for now
        if (left_reg->size > 1) {
            error(node->token, "only 8-bit operations supported for now");
        }

        // Move left out of accumulator if necessary
        if (strcmp(left_reg->name, "a") == 0) {
            left_reg = allocate_reg(1);
            out_pointer += sprintf(out_pointer, "\tmov %s, a\n", left_reg->name);
            out_pointer += sprintf(out_pointer, "\tmov a, 0\n");
            out_pointer += sprintf(out_pointer, "\tsub %s\n", left_reg->name);
            out_pointer += sprintf(out_pointer, "\tmov a, %s\n", left_reg->name);
            free_reg(left_reg);
            left_reg = &registers[0];
        } else {
            out_pointer += sprintf(out_pointer, "\tmov a, 0\n");
            out_pointer += sprintf(out_pointer, "\tsub %s\n", left_reg->name);
        }

        return left_reg;
    } else if (strcmp(node->token->value, "*") == 0) {
        struct Register* pointer_reg = get_address(node, depth);
        free_reg(pointer_reg);
        struct Register* value_reg = allocate_reg(node->UnaryOp.left->Variable.symbol->type->base->size);

        out_pointer += sprintf(out_pointer, "\tmov %s, [%s]\n", value_reg->name, pointer_reg->name);
        
        return value_reg;
    } else if (strcmp(node->token->value, "&") == 0) {
        printf("%s\t", node->type->name);
        print_indent(depth);
        printf("UnaryOp: %s\n", node->token->value);

        struct Register* pointer_reg = get_address(node->UnaryOp.left, depth+1);

        return pointer_reg;
    } else {
        error(node->token, "invalid unary operator");
    }
}

static void visit_return(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Return\n");

    // Get return value
    struct Register* reg = visit(node->Return.expr, depth+1);

    // Move return value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) out_pointer += sprintf(out_pointer, "\tmov a, %s\n", reg->name);

    // TODO this needs account for returns from inside futher scopes
    // Restore stack
    if (node->scope->stack_size != 0) out_pointer += sprintf(out_pointer, "\tmov sp, sp+%d\n", node->scope->stack_size);

    out_pointer += sprintf(out_pointer, "\tret\n");
    free_reg(reg);
}

static void visit_if(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("If\n");

    static int label_count = 0;

    // Get test value
    struct Register* reg = visit(node->Return.expr, depth+1);

    // Push accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        out_pointer += sprintf(out_pointer, "\tpush a\n");
        local_stack_usage += 1;
    }

    // Move test value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) out_pointer += sprintf(out_pointer, "\tmov a, %s\n", reg->name);

    // Compare
    out_pointer += sprintf(out_pointer, "\tcmp 0\n");

    // Restore accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        out_pointer += sprintf(out_pointer, "\tpop a\n");
        local_stack_usage -= 1;
    }

    // Jump if equal 0
    out_pointer += sprintf(out_pointer, "\tje .if_false_%d\n", label_count);
    free_reg(reg);

    // Visit true branch
    out_pointer += sprintf(out_pointer, ".if_true_%d:\n", label_count);
    visit(node->If.true_statement, depth+1);
    out_pointer += sprintf(out_pointer, "\tjmp .if_exit_%d\n", label_count);

    // Visit false branch
    out_pointer += sprintf(out_pointer, ".if_false_%d:\n", label_count);
    if (node->If.false_statement != NULL) visit(node->If.false_statement, depth+1);

    out_pointer += sprintf(out_pointer, ".if_exit_%d:\n", label_count);

    label_count++;
}

static void visit_while(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("While\n");

    static int label_count = 0;

    out_pointer += sprintf(out_pointer, ".while_start_%d:\n", label_count);

    // Get test value
    struct Register* reg = visit(node->Return.expr, depth+1);

    // Push accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        out_pointer += sprintf(out_pointer, "\tpush a\n");
        local_stack_usage += 1;
    }

    // Move test value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) out_pointer += sprintf(out_pointer, "\tmov a, %s\n", reg->name);

    // Compare
    out_pointer += sprintf(out_pointer, "\tcmp 0\n");

    // Restore accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        out_pointer += sprintf(out_pointer, "\tpop a\n");
        local_stack_usage -= 1;
    }

    // Jump if equal 0
    out_pointer += sprintf(out_pointer, "\tje .while_exit_%d\n", label_count);
    free_reg(reg);

    // Visit loop statement
    out_pointer += sprintf(out_pointer, ".while_contents_%d:\n", label_count);
    visit(node->While.loop_statement, depth+1);

    // Go back to start of loop
    out_pointer += sprintf(out_pointer, "\tjmp .while_start_%d\n", label_count);

    out_pointer += sprintf(out_pointer, ".while_exit_%d:\n", label_count);

    label_count++;
}

static struct Register* visit_func_call(struct Node* node, int depth) {
    printf("%s\t", node->type->name);
    print_indent(depth);
    printf("Call: %s\n", node->token->value);

    // Push accumulator if necessary
    int preserve_a = !registers[0].free;
    if (preserve_a) {
        out_pointer += sprintf(out_pointer, "\tpush a\n");
        local_stack_usage += 1;
    }

    out_pointer += sprintf(out_pointer, "\tcall %s\n", node->token->value);

    // Move result out of accumulator if necessary
    struct Register* result_reg = allocate_reg(1);
    if (preserve_a) out_pointer += sprintf(out_pointer, "\tmov %s, a\n", result_reg->name);

    // Restore accumulator if necessary
    if (preserve_a) {
        out_pointer += sprintf(out_pointer, "\tpop a\n");
        local_stack_usage -= 1;
    }

    return result_reg;
}

static struct Register* visit(struct Node* node, int depth) {
    switch (node->kind) {
        case N_PROGRAM:
            visit_program(node, depth);
            return NULL;
        case N_VAR_DECL:
            visit_var_decl(node, depth);
            return NULL;
        case N_FUNC_DECL:
            visit_func_decl(node, depth);
            return NULL;
        case N_BLOCK:
            visit_block(node, depth);
            return NULL;
        case N_NUMBER:
            return visit_number(node, depth);
        case N_VARIABLE:
            return visit_variable(node, depth);
        case N_ASSIGNMENT:
            visit_assignment(node, depth);
            return NULL;
        case N_BINOP:
            return visit_bin_op(node, depth);
        case N_UNARY:
            return visit_unary_op(node, depth);
        case N_RETURN:
            visit_return(node, depth);
            return NULL;
        case N_IF:
            visit_if(node, depth);
            return NULL;
        case N_WHILE:
            visit_while(node, depth);
            return NULL;
        case N_FUNC_CALL:
            return visit_func_call(node, depth);
    }

    error(node->token, "invalid node kind");
    return NULL;
}

char* generate(struct Node* root_node) {
    // Program entry code
    out_pointer += sprintf(out_pointer, "#include \"architecture.asm\"\n\n");
    out_pointer += sprintf(out_pointer, "#bank RAM\n\n");
    out_pointer += sprintf(out_pointer, "#addr 0x8100\n\n");
    out_pointer += sprintf(out_pointer, "call main\n");
    out_pointer += sprintf(out_pointer, "ret\n\n");

    visit(root_node, 0);
    
    return out_buffer;
}