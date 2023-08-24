#include "generator.h"

static int visit(Node*, int);

static char intermediateCode [1024];
static char* codePointer = intermediateCode;

static int registerUse[8] = {0};
static char* registerNames[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int allocReg() {
    for (int i = 0; i < sizeof(registerUse)/sizeof(int); i++) {
        if (registerUse[i] == 0) {
            registerUse[i] = 1;
            return i;
        }
    }
}

static void freeReg(int i) {
    registerUse[i] = 0;
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

static int visit(Node* node, int depth) {
    printIndent(depth);
    switch (node->type) {
        case N_VAR_DECL:
            // TODO decide symbol location in memory
            printf("Variable declaration: %s %s\n", node->VarDecl.symbol->type->name, node->VarDecl.symbol->token->value);
            break;
        case N_FUNC_DECL:
            printf("Function declaration: %s %s\n", node->FunctionDecl.returnType->name, node->token->value);
            codePointer += sprintf(codePointer, "LABEL %s\n", node->token->value);
            visitAll(node->FunctionDecl.formalParameters, depth+1);
            visit(node->FunctionDecl.block, depth+1);
            break;
        case N_BLOCK:
            printf("Block\n");
            visitAll(node->Block.variableDeclarations, depth+1);
            visitAll(node->Block.statements, depth+1);
            break;
        case N_NUMBER:
            printf("Number: %s\n", node->token->value);
            int regN = allocReg();
            codePointer += sprintf(codePointer, "COPY %s -> R%d %s\n", node->token->value, regN);
            return regN;
        case N_VARIABLE:
            printf("Variable: %s\n", node->token->value);
            int regV = allocReg();
            // TODO use symbol location instead of token value
            codePointer += sprintf(codePointer, "COPY %s -> R%d\n", node->token->value, regV);
            return regV;
        case N_ASSIGNMENT:
            printf("Assignment: %s\n", node->token->value);
            int regA = visit(node->Assignment.expr, depth+1);
            codePointer += sprintf(codePointer, "COPY R%d -> %s\n", regA, node->token->value);
            break;
        case N_BINOP:
            printf("BinOp: %s\n", node->token->value);
            int regL = visit(node->BinOp.left, depth+1);
            int regR = visit(node->BinOp.right, depth+1);
            int regRes = allocReg();
            codePointer += sprintf(codePointer, "BINOP R%d %s R%d -> R%d\n", regL, node->token->value, regR, regRes);
            freeReg(regL);
            freeReg(regR);
            return regRes;
        case N_RETURN:
            printf("Return\n");
            int reg = visit(node->Return.expr, depth+1);
            codePointer += sprintf(codePointer, "RET R%d\n", reg);
            return reg;
        default:
            printf("Unknown node type!\n");
            break;
    }

    return 0;
}

char* generate(Node* rootNode) {
    visit(rootNode, 0);
    return intermediateCode;
}