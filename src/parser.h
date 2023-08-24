#ifndef _PARSER_H
#define _PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "symbol.h"

enum NodeType {N_TYPE, N_VAR_DECL, N_FUNC_DECL, N_BLOCK, N_VARIABLE, N_NUMBER, N_ASSIGNMENT, N_BINOP, N_RETURN};

struct NodeListEntry;
typedef struct NodeListEntry NodeListEntry;

typedef struct Node Node;
struct Node {
    Token* token;
    enum NodeType type;

    union {
        struct {
            Symbol* symbol;
        } VarDecl;
        struct {
            Type* returnType;
            Node* block;
            NodeListEntry* formalParameters;
        } FunctionDecl;
        struct {
            NodeListEntry* statements;
            NodeListEntry* variableDeclarations;
        } Block;
        struct {
            Symbol* symbol;
        } Variable;
        struct {
            Node* variable;
            Node* expr;
        } Assignment;
        struct {
            Node* left;
            Node* right;
        } BinOp;
        struct {
            Node* expr;
        } Return;
    };
};


struct NodeListEntry {
    Node* node;
    NodeListEntry* next;
};

Node* parse(Token*);

#endif