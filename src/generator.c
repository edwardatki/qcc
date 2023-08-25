#include "generator.h"
#include "register.h"
#include "scope.h"

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

static Register registers[] = { {.name="a", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="b", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="c", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="d", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="e", .size=1, .free=1, .subReg={NULL, NULL}},
                                {.name="bc", .size=2, .free=1, .subReg={&registers[1], &registers[2]}},
                                {.name="de", .size=2, .free=1, .subReg={&registers[3], &registers[4]}}};

static Register* allocReg(int size) {
    for (int i = 0; i < sizeof(registers)/sizeof(Register); i++) {
        Register* reg = &registers[i];
        if ((reg->size >= size) && reg->free) {
            // If sub registers are in use then can't allocate
            if ((reg->subReg[0] != NULL) && !reg->subReg[0]->free) break;
            if ((reg->subReg[1] != NULL) && !reg->subReg[1]->free) break;

            // Mark register as used
            reg->free = 0;

            // Mark sub registers as used too
            if (reg->subReg[0] != NULL) reg->subReg[0]->free = 0;
            if (reg->subReg[1] != NULL) reg->subReg[1]->free = 0;

            // printf("ALLOCATED REGISTER %s\n", reg->name);
            return reg;
        }
    }

    printf("UNABLE TO ALLOCATE REGISTER\n");
    return NULL;
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
        visit(curEntry->node, depth);
    }
}

static void printIndent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf(" ");
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
    outputPointer += sprintf(outputPointer, "\n");
}

static void visitBlock(Node* node, int depth) {
    printIndent(depth);
    printf("Block\n");

    visitAll(node->Block.variableDeclarations, depth+1);
    visitAll(node->Block.statements, depth+1);
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

    Register* pointerReg = allocReg(2);
    Symbol* symbol = node->Variable.symbol;
    outputPointer += sprintf(outputPointer, "\tmov %s, %s\n", pointerReg->name, symbol->location);

    Register* reg = allocReg(1);
    outputPointer += sprintf(outputPointer, "\tmov %s, [%s]\n", reg->name, pointerReg->name);

    freeReg(pointerReg);

    return reg;
}

static void visitAssignment(Node* node, int depth) {
    printIndent(depth);
    printf("Assignment: %s\n", node->token->value);

    Register* reg = visit(node->Assignment.expr, depth+1);
    Register* pointerReg = allocReg(2);
    Symbol* symbol = node->Assignment.variable->Variable.symbol;
    
    outputPointer += sprintf(outputPointer, "\tmov %s, %s\n", pointerReg->name, symbol->location);
    outputPointer += sprintf(outputPointer, "\tmov [%s], %s\n", pointerReg->name, reg->name);  
    
    freeReg(reg);
    freeReg(pointerReg);
}

static Register* visitBinop(Node* node, int depth) {
    printIndent(depth);
    printf("BinOp: %s\n", node->token->value);

    Register* regL = visit(node->BinOp.left, depth+1);
    Register* regR = visit(node->BinOp.right, depth+1);

    // Move left into accumulator if necessary
    if (strcmp(regL->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov a, %s\n", regL->name);

    // Decide opcode
    char opcode[4] = "???";
    if (strcmp(node->token->value, "+") == 0) strcpy(opcode, "add");
    else if (strcmp(node->token->value, "-") == 0) strcpy(opcode, "sub");

    // Push accumulator if necessary
    if ((strcmp(regL->name, "a") != 0) && (!registers[0].free)) outputPointer += sprintf(outputPointer, "\tpush a\n");

    // Add right to accumulator
    outputPointer += sprintf(outputPointer, "\t%s %s\n", opcode, regR->name);
    freeReg(regR);

    // Move result from accumulator back to left if necessary
    if (strcmp(regL->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov %s, a\n", regL->name);

    // Restore accumulator if necessary
    if ((strcmp(regL->name, "a") != 0) && (!registers[0].free)) outputPointer += sprintf(outputPointer, "\tpop a\n");

    return regL;
}

static void visitReturn(Node* node, int depth) {
    printIndent(depth);
    printf("Return\n");

    // Get return value
    Register* reg = visit(node->Return.expr, depth+1);

    // Move return value to accumulator if necessary
    if (strcmp(reg->name, "a") != 0) outputPointer += sprintf(outputPointer, "\tmov a, %s\n", reg->name);

    outputPointer += sprintf(outputPointer, "\tret\n");
}

static Register* visit(Node* node, int depth) {
    switch (node->type) {
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
            return visitBinop(node, depth);
        case N_RETURN:
            visitReturn(node, depth);
            return NULL;
    }

    return NULL;
}

char* generate(Node* rootNode) {
    // Program entry code
    outputPointer += sprintf(outputPointer, "mov sp, 0x7fff\n");
    outputPointer += sprintf(outputPointer, "call main\n\n");

    visitFuncDecl(rootNode, 0);
    
    return outputBuffer;
}