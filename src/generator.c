#include "generator.h"
#include "register.h"
#include "scope.h"
#include "print_formatting.h"

static void visitVarDecl(Node*, int);
static void visitFuncDecl(Node*, int);
static void visitBlock(Node*, int);
static Register* visitNumber(Node*, int);
static Register* visitVariable(Node*, int);
static void visitAssignment(Node*, int);
static Register* visitBinop(Node*, int);
static void visitReturn(Node*, int);
static Register* visit(Node*, int);

static char outputBuffer [1024];
static char* outputPointer = outputBuffer;

int localStackUsage = 0;

static Register registers[] = { {.name="a", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="b", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="c", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="d", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="e", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="bc", .size=2, .free=1, .subReg={&registers[1], &registers[2]}},
                                {.name="de", .size=2, .free=1, .subReg={&registers[3], &registers[4]}}};

static Register* allocReg(int size) {
    for (int i = 0; i < sizeof(registers)/sizeof(Register); i++) {
        // if (i == 0) continue; // TEMPORARY don't allocate a so we never have to push it to the stack
        Register* reg = &registers[i];
        if ((reg->size >= size) && reg->free) {
            // If sub registers are in use then can't allocate
            if ((reg->subReg[0] != NULL) && !reg->subReg[0]->free) continue;
            if ((reg->subReg[1] != NULL) && !reg->subReg[1]->free) continue;

            // Mark register as used
            reg->free = 0;

            // Mark sub registers as used too
            if (reg->subReg[0] != NULL) reg->subReg[0]->free = 0;
            if (reg->subReg[1] != NULL) reg->subReg[1]->free = 0;

            // printf("ALLOCATED REGISTER %s\n", reg->name);
            return reg;
        }
    }

    printf("%serror:%s unable to allocate %d byte register\n", RED, RESET, size);
    printf("a: %s\n", registers[0].free ? GRN "free" RESET : RED "used" RESET);
    printf("b: %s\n", registers[1].free ? GRN "free" RESET : RED "used" RESET);
    printf("c: %s\n", registers[2].free ? GRN "free" RESET : RED "used" RESET);
    printf("d: %s\n", registers[3].free ? GRN "free" RESET : RED "used" RESET);
    printf("e: %s\n", registers[4].free ? GRN "free" RESET : RED "used" RESET);
    printf("bc: %s\n", registers[5].free ? GRN "free" RESET : RED "used" RESET);
    printf("de: %s\n", registers[6].free ? GRN "free" RESET : RED "used" RESET);
    exit(EXIT_FAILURE);
}

static void freeReg(Register* reg) {
    if (reg == NULL) return;

    // Mark register as free
    reg->free = 1;

    // Mark sub registers as free too
    if (reg->subReg[0] != NULL) reg->subReg[0]->free = 1;
    if (reg->subReg[1] != NULL) reg->subReg[1]->free = 1;

    // printf("FREED REGISTER %s\n", reg->name);
}

static void visitAll(NodeListEntry* listRoot, int depth) {
    if (listRoot == NULL) return;
    NodeListEntry* curEntry = listRoot;
    while (curEntry->next != NULL) {
        curEntry = curEntry->next;
        Register* reg = visit(curEntry->node, depth);
        freeReg(reg);   // Free registers that were allocated but never used for anything :(
    }
}

static void printIndent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }
}

static void loadVariable(Register* reg, Scope* scope, Symbol* symbol) {

    Register* pointerReg = allocReg(2);

    if (symbol->global) {
        outputPointer += sprintf(outputPointer, "\tmov %s, %s\n", pointerReg->name, symbol->token->value);
    } else {
        outputPointer += sprintf(outputPointer, "\tmov %s, sp+%d\n", pointerReg->name, getSymbolStackOffset(symbol, scope)+localStackUsage);
    }
    outputPointer += sprintf(outputPointer, "\tmov %s, [%s]\n", reg->name, pointerReg->name);  

    freeReg(pointerReg);
}

static void storeVariable(Register* reg, Scope* scope, Symbol* symbol) {
    Register* pointerReg = allocReg(2);

    if (symbol->global) {
        outputPointer += sprintf(outputPointer, "\tmov %s, %s\n", pointerReg->name, symbol->token->value);
    } else {
        outputPointer += sprintf(outputPointer, "\tmov %s, sp+%d\n", pointerReg->name, getSymbolStackOffset(symbol, scope)+localStackUsage);
    }
    outputPointer += sprintf(outputPointer, "\tmov [%s], %s\n", pointerReg->name, reg->name);  

    freeReg(pointerReg);
}

static char* getVariableLocation(Scope* scope, Symbol* symbol) {
    if (symbol->global) {
        return symbol->token->value;
    } else {
        char* location = calloc(32, sizeof(char));
        sprintf(location, "sp+%d", getSymbolStackOffset(symbol, scope)+localStackUsage);
        return location;
    }
}

static void visitVarDecl(Node* node, int depth) {
    printIndent(depth);
    printf("Variable declaration: %s %s\n", node->VarDecl.symbol->type->name, node->VarDecl.symbol->token->value);
    
    Symbol* symbol = node->VarDecl.symbol;

    // Will need to reserve space for global variables and track local variable's positions on the stack
}

static void visitFuncDecl(Node* node, int depth) {
    // Only bother with main for now
    if (strcmp(node->token->value, "main") != 0) return;

    printIndent(depth);
    printf("Function declaration: %s %s\n", node->FunctionDecl.returnType->name, node->token->value);

    outputPointer += sprintf(outputPointer, "%s:\n", node->token->value);

    visitAll(node->FunctionDecl.formalParameters, depth+1);
    visit(node->FunctionDecl.block, depth+1);

    outputPointer += sprintf(outputPointer, ".%s_exit:\n", node->token->value);
    outputPointer += sprintf(outputPointer, "\tret\n\n");
}

static void visitBlock(Node* node, int depth) {
    printIndent(depth);
    printf("Block\n");

    // Allocate stack space
    if (node->scope->stackSize != 0) outputPointer += sprintf(outputPointer, "\tmov sp, sp-%d\n", node->scope->stackSize);

    visitAll(node->Block.variableDeclarations, depth+1);
    visitAll(node->Block.statements, depth+1);

    // Deallocate
    if (node->scope->stackSize != 0) outputPointer += sprintf(outputPointer, "\tmov sp, sp+%d\n", node->scope->stackSize);
}

static Register* visitNumber(Node* node, int depth) {
    printIndent(depth);
    printf("Number: %s\n", node->token->value);

    Register* reg = allocReg(1);
    outputPointer += sprintf(outputPointer, "\tmov %s, %s\n", reg->name, node->token->value);
    return reg;
}

static Register* visitVariable(Node* node, int depth) {
    printIndent(depth);
    printf("Variable: %s\n", node->token->value);

    Register* reg = allocReg(1);
    Symbol* symbol = node->Variable.symbol;

    loadVariable(reg, node->scope, symbol);

    return reg;
}

static void visitAssignment(Node* node, int depth) {
    printIndent(depth);
    printf("Assignment: %s\n", node->token->value);

    Register* reg = visit(node->Assignment.expr, depth+1);
    Symbol* symbol = node->Assignment.variable->Variable.symbol;

    storeVariable(reg, node->scope, symbol);
    
    freeReg(reg);
}

static Register* visitBinOp(Node* node, int depth) {
    printIndent(depth);
    printf("BinOp: %s\n", node->token->value);

    Register* regL = visit(node->BinOp.left, depth+1);
    Register* regR = visit(node->BinOp.right, depth+1);

    // Can only do 8-bit operations for now
    if ((regL->size > 1) | (regR->size > 1)) {
        printf("%d:%d %serror:%s only 8-bit operations supported\n", node->UnaryOp.left->token->line,node->UnaryOp.left->token->column, RED, RESET);
        exit(EXIT_FAILURE);
    }

    // Push accumulator if necessary
    if ((strcmp(regL->name, "a") != 0) && (!registers[0].free)) {
        outputPointer += sprintf(outputPointer, "\tpush a\n");
        localStackUsage += 1;
    }

    // Move left into accumulator if necessary
    if (strcmp(regL->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov a, %s\n", regL->name);

    // Perform operation
    if (strcmp(node->token->value, "+") == 0) {
        outputPointer += sprintf(outputPointer, "\tadd %s\n", regR->name);
    } else if (strcmp(node->token->value, "-") == 0) {
        outputPointer += sprintf(outputPointer, "\tsub %s\n", regR->name);
    } else if (strcmp(node->token->value, ">") == 0) {
        static int labelCounter = 0;
        
        outputPointer += sprintf(outputPointer, "\tcmp %s\n", regR->name);
        outputPointer += sprintf(outputPointer, "\tjnc .cmp_more_false_%d\n", labelCounter); // A <= right

        outputPointer += sprintf(outputPointer, ".cmp_more_true_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 1\n");
        outputPointer += sprintf(outputPointer, "\tjmp .cmp_more_exit_%d\n", labelCounter);

        outputPointer += sprintf(outputPointer, ".cmp_more_false_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 0\n");

        outputPointer += sprintf(outputPointer, ".cmp_more_exit_%d:\n", labelCounter);

        labelCounter++;
    } else if (strcmp(node->token->value, "<") == 0) {
        static int labelCounter = 0;
        
        outputPointer += sprintf(outputPointer, "\tcmp %s\n", regR->name);
        outputPointer += sprintf(outputPointer, "\tjc .cmp_less_false_%d\n", labelCounter);  // A > right
        outputPointer += sprintf(outputPointer, "\tje .cmp_less_false_%d\n", labelCounter);  // A == right

        outputPointer += sprintf(outputPointer, ".cmp_less_true_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 1\n");
        outputPointer += sprintf(outputPointer, "\tjmp .cmp_less_exit_%d\n", labelCounter);

        outputPointer += sprintf(outputPointer, ".cmp_less_false_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 0\n");

        outputPointer += sprintf(outputPointer, ".cmp_less_exit_%d:\n", labelCounter);
    } else if (strcmp(node->token->value, ">=") == 0) {
        static int labelCounter = 0;
        
        outputPointer += sprintf(outputPointer, "\tcmp %s\n", regR->name);
        outputPointer += sprintf(outputPointer, "\tjne .cmp_more_equal_true_%d\n", labelCounter); // A == right
        outputPointer += sprintf(outputPointer, "\tjnc .cmp_more_equal_false_%d\n", labelCounter); // A <= right

        outputPointer += sprintf(outputPointer, ".cmp_more_equal_true_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 1\n");
        outputPointer += sprintf(outputPointer, "\tjmp .cmp_more_exit_%d\n", labelCounter);

        outputPointer += sprintf(outputPointer, ".cmp_more_equal_false_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 0\n");

        outputPointer += sprintf(outputPointer, ".cmp_more_equal_exit_%d:\n", labelCounter);
    } else if (strcmp(node->token->value, "<=") == 0) {
        static int labelCounter = 0;
        
        outputPointer += sprintf(outputPointer, "\tcmp %s\n", regR->name);
        outputPointer += sprintf(outputPointer, "\tjc .cmp_less_equal_false_%d\n", labelCounter); // A > right

        outputPointer += sprintf(outputPointer, ".cmp_less_equal_true_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 1\n");
        outputPointer += sprintf(outputPointer, "\tjmp .cmp_less_equal_exit_%d\n", labelCounter);

        outputPointer += sprintf(outputPointer, ".cmp_less_equal_false_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 0\n");

        outputPointer += sprintf(outputPointer, ".cmp_less_equal_exit_%d:\n", labelCounter);
    } else if (strcmp(node->token->value, "==") == 0) {
        static int labelCounter = 0;
        
        outputPointer += sprintf(outputPointer, "\tcmp %s\n", regR->name);
        outputPointer += sprintf(outputPointer, "\tjne .cmp_equal_false_%d\n", labelCounter); // A != right

        outputPointer += sprintf(outputPointer, ".cmp_equal_true_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 1\n");
        outputPointer += sprintf(outputPointer, "\tjmp .cmp_equal_exit_%d\n", labelCounter);

        outputPointer += sprintf(outputPointer, ".cmp_equal_false_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 0\n");

        outputPointer += sprintf(outputPointer, ".cmp_equal_exit_%d:\n", labelCounter);
    } else if (strcmp(node->token->value, "!=") == 0) {
        static int labelCounter = 0;
        
        outputPointer += sprintf(outputPointer, "\tcmp %s\n", regR->name);
        outputPointer += sprintf(outputPointer, "\tje .cmp_not_equal_false_%d\n", labelCounter); // A != right

        outputPointer += sprintf(outputPointer, ".cmp_not_equal_true_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 1\n");
        outputPointer += sprintf(outputPointer, "\tjmp .cmp_not_equal_exit_%d\n", labelCounter);

        outputPointer += sprintf(outputPointer, ".cmp_not_equal_false_%d:\n", labelCounter);
        outputPointer += sprintf(outputPointer, "\tmov a, 0\n");

        outputPointer += sprintf(outputPointer, ".cmp_not_equal_exit_%d:\n", labelCounter);
    } else {
        outputPointer += sprintf(outputPointer, "\t???\n");
    }
    
    freeReg(regR);

    // Move result from accumulator back to left if necessary
    if (strcmp(regL->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov %s, a\n", regL->name);

    // Restore accumulator if necessary
    if ((strcmp(regL->name, "a") != 0) && (!registers[0].free)) {
        outputPointer += sprintf(outputPointer, "\tpop a\n");
        localStackUsage -= 1;
    }

    return regL;
}

static Register* visitUnaryOp(Node* node, int depth) {
    printIndent(depth);
    printf("UnaryOp: %s\n", node->token->value);

    // Perform operation
    if (strcmp(node->token->value, "+") == 0) {
        // Don't need to do anything here
        Register* regL = visit(node->UnaryOp.left, depth+1);
        return regL;
    } else if (strcmp(node->token->value, "-") == 0) {
        Register* regL = visit(node->UnaryOp.left, depth+1);

        // Can only do 8-bit operations for now
        if (regL->size > 1) {
            printf("%d:%d %serror:%s only 8-bit operations supported\n", node->UnaryOp.left->token->line,node->UnaryOp.left->token->column, RED, RESET);
            exit(EXIT_FAILURE);
        }

        // Move left out of accumulator if necessary
        if (strcmp(regL->name, "a") == 0) {
            regL = allocReg(1);
            outputPointer += sprintf(outputPointer, "\tmov %s, a\n", regL->name);
            outputPointer += sprintf(outputPointer, "\tmov a, 0\n");
            outputPointer += sprintf(outputPointer, "\tsub %s\n", regL->name);
            outputPointer += sprintf(outputPointer, "\tmov a, %s\n", regL->name);
            freeReg(regL);
            regL = &registers[0];
        } else {
            outputPointer += sprintf(outputPointer, "\tmov a, 0\n");
            outputPointer += sprintf(outputPointer, "\tsub %s\n", regL->name);
        }

        return regL;
    } else if (strcmp(node->token->value, "*") == 0) {
        Symbol* symbol = node->UnaryOp.left->Variable.symbol;

        printIndent(depth+1);
        printf("Variable: %s\n", symbol->token->value);

        Register* regL = allocReg(symbol->type->base->size);
        loadVariable(regL, node->scope, symbol);
        
        return regL;
    } else if (strcmp(node->token->value, "&") == 0) {
        Symbol* symbol = node->UnaryOp.left->Variable.symbol;

        printIndent(depth+1);
        printf("Variable: %s\n", symbol->token->value);

        Register* pointerReg = allocReg(pointerTo(symbol->type)->size);
        outputPointer += sprintf(outputPointer, "\tmov %s, %s\n", pointerReg->name, getVariableLocation(node->scope, symbol));

        return pointerReg;
    } else {
        outputPointer += sprintf(outputPointer, "\t???\n");
        return NULL;
    }
}

static void visitReturn(Node* node, int depth) {
    printIndent(depth);
    printf("Return\n");

    // Get return value
    Register* reg = visit(node->Return.expr, depth+1);

    // Move return value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov a, %s\n", reg->name);

    // TODO this needs account for returns from inside futher scopes
    // Restore stack
    outputPointer += sprintf(outputPointer, "\tmov sp, sp+%d\n", node->scope->stackSize);

    outputPointer += sprintf(outputPointer, "\tret\n");
    freeReg(reg);
}

static void visitIf(Node* node, int depth) {
    printIndent(depth);
    printf("If\n");

    static int labelCounter = 0;

    // Get test value
    Register* reg = visit(node->Return.expr, depth+1);

    // Push accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        outputPointer += sprintf(outputPointer, "\tpush a\n");
        localStackUsage += 1;
    }

    // Move test value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov a, %s\n", reg->name);

    // Compare
    outputPointer += sprintf(outputPointer, "\tcmp 0\n");

    // Restore accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        outputPointer += sprintf(outputPointer, "\tpop a\n");
        localStackUsage -= 1;
    }

    // Jump if equal 0
    outputPointer += sprintf(outputPointer, "\tje .if_false_%d\n", labelCounter);
    freeReg(reg);

    // Visit true branch
    outputPointer += sprintf(outputPointer, ".if_true_%d:\n", labelCounter);
    visit(node->If.true_statement, depth+1);
    outputPointer += sprintf(outputPointer, "\tjmp .if_exit_%d\n", labelCounter);

    // Visit false branch
    outputPointer += sprintf(outputPointer, ".if_false_%d:\n", labelCounter);
    if (node->If.false_statement != NULL) visit(node->If.false_statement, depth+1);

    outputPointer += sprintf(outputPointer, ".if_exit_%d:\n", labelCounter);

    labelCounter++;
}

static void visitWhile(Node* node, int depth) {
    printIndent(depth);
    printf("While\n");

    static int labelCounter = 0;

    outputPointer += sprintf(outputPointer, ".while_start_%d:\n", labelCounter);

    // Get test value
    Register* reg = visit(node->Return.expr, depth+1);

    // Push accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        outputPointer += sprintf(outputPointer, "\tpush a\n");
        localStackUsage += 1;
    }

    // Move test value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov a, %s\n", reg->name);

    // Compare
    outputPointer += sprintf(outputPointer, "\tcmp 0\n");

    // Restore accumulator if necessary
    if ((strcmp(reg->name, "a") != 0) && (!registers[0].free)) {
        outputPointer += sprintf(outputPointer, "\tpop a\n");
        localStackUsage -= 1;
    }

    // Jump if equal 0
    outputPointer += sprintf(outputPointer, "\tje .while_exit_%d\n", labelCounter);
    freeReg(reg);

    // Visit loop statement
    outputPointer += sprintf(outputPointer, ".while_contents_%d:\n", labelCounter);
    visit(node->While.loop_statement, depth+1);

    // Go back to start of loop
    outputPointer += sprintf(outputPointer, "\tjmp .while_start_%d\n", labelCounter);

    outputPointer += sprintf(outputPointer, ".while_exit_%d:\n", labelCounter);

    labelCounter++;
}

static Register* visit(Node* node, int depth) {
    switch (node->kind) {
        case N_VAR_DECL:
            visitVarDecl(node, depth);
            return NULL;
        case N_FUNC_DECL:
            visitFuncDecl(node, depth);
            return NULL;
        case N_BLOCK:
            visitBlock(node, depth);
            return NULL;
        case N_NUMBER:
            return visitNumber(node, depth);
        case N_VARIABLE:
            return visitVariable(node, depth);
        case N_ASSIGNMENT:
            visitAssignment(node, depth);
            return NULL;
        case N_BINOP:
            return visitBinOp(node, depth);
        case N_UNARY:
            return visitUnaryOp(node, depth);
        case N_RETURN:
            visitReturn(node, depth);
            return NULL;
        case N_IF:
            visitIf(node, depth);
            return NULL;
        case N_WHILE:
            visitWhile(node, depth);
            return NULL;
    }

    return NULL;
}

char* generate(Node* rootNode) {
    // Program entry code
    outputPointer += sprintf(outputPointer, "#bank RAM\n\n");
    outputPointer += sprintf(outputPointer, "mov sp, 0x7fff\n");
    outputPointer += sprintf(outputPointer, "call main\n");
    outputPointer += sprintf(outputPointer, "call print_u8_dec\n");
    outputPointer += sprintf(outputPointer, "jmp $\n\n");
    outputPointer += sprintf(outputPointer, "#include \"architecture.asm\"\n");
    outputPointer += sprintf(outputPointer, "#include \"print_functions.asm\"\n\n");

    visitFuncDecl(rootNode, 0);
    
    return outputBuffer;
}