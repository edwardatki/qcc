#ifndef _PARSER_H
#define _PARSER_H

struct Token;
struct Symbol;
struct Type;
struct Scope;
struct List;

enum NodeKind {N_TYPE, N_PROGRAM, N_VAR_DECL, N_FUNC_DECL, N_BLOCK, N_VARIABLE, N_NUMBER, N_ASSIGNMENT, N_BINOP, N_UNARY, N_RETURN, N_IF, N_WHILE, N_FUNC_CALL, N_STRING};

struct Node {
    struct Token* token;
    enum NodeKind kind;
    struct Type* type;
    struct Scope* scope;

    int constant;

    union {
        struct {
            struct List* function_declarations;
            struct List* global_variables;
        } Program;
        struct {
            struct Symbol* symbol;
            struct Node* assignment;
        } VarDecl;
        struct {
            struct Node* block;
            struct List* formal_parameters;
        } FunctionDecl;
        struct {
            struct List* statements;
            struct List* variable_declarations;
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
        struct {
            struct Symbol* symbol;
            struct List* parameters;
        } FuncCall;
    };
};

struct Node* parse(struct Token*);

#endif