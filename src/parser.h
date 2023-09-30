#ifndef _PARSER_H
#define _PARSER_H

struct Token;
struct Symbol;
struct Type;
struct Scope;

enum NodeKind {N_TYPE, N_PROGRAM, N_VAR_DECL, N_FUNC_DECL, N_BLOCK, N_VARIABLE, N_NUMBER, N_ASSIGNMENT, N_BINOP, N_UNARY, N_RETURN, N_IF, N_WHILE};

struct NodeListEntry;

struct Node {
    struct Token* token;
    enum NodeKind kind;
    struct Type* type;
    struct Scope* scope;

    union {
        struct {
            struct NodeListEntry* function_declarations;
            struct NodeListEntry* global_variables;
        } Program;
        struct {
            struct Symbol* symbol;
        } VarDecl;
        struct {
            struct Node* block;
            struct NodeListEntry* formal_parameters;
        } FunctionDecl;
        struct {
            struct NodeListEntry* statements;
            struct NodeListEntry* variable_declarations;
        } Block;
        struct {
            struct Symbol* symbol;
        } Variable;
        struct {
            struct Node* left;
            struct Node* right;
        } Assignment;
        struct {
            struct Node* left;
            struct Node* right;
        } BinOp;
        struct {
            struct Node* left;
            struct Node* right;
        } UnaryOp;
        struct {
            struct Node* expr;
        } Return;
        struct {
            struct Node* expr;
            struct Node* true_statement;
            struct Node* false_statement;
        } If;
        struct {
            struct Node* expr;
            struct Node* loop_statement;
        } While;
    };
};


struct NodeListEntry {
    struct Node* node;
    struct NodeListEntry* next;
};

struct Node* parse(struct Token*);

#endif