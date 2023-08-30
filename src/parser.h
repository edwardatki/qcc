#ifndef _PARSER_H
#define _PARSER_H

struct Token;
typedef struct Token Token;

struct Symbol;
typedef struct Symbol Symbol;

struct Type;
typedef struct Type Type;

struct Scope;
typedef struct Scope Scope;

enum NodeKind {N_TYPE, N_VAR_DECL, N_FUNC_DECL, N_BLOCK, N_VARIABLE, N_NUMBER, N_ASSIGNMENT, N_BINOP, N_UNARY, N_RETURN, N_IF, N_WHILE};

struct NodeListEntry;
typedef struct NodeListEntry NodeListEntry;

typedef struct Node Node;
struct Node {
    Token* token;
    enum NodeKind kind;
    Type* type;
    Scope* scope;

    union {
        struct {
            Symbol* symbol;
        } VarDecl;
        struct {
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
            Node* left;
            Node* right;
        } UnaryOp;
        struct {
            Node* expr;
        } Return;
        struct {
            Node* expr;
            Node* true_statement;
            Node* false_statement;
        } If;
        struct {
            Node* expr;
            Node* loop_statement;
        } While;
    };
};


struct NodeListEntry {
    Node* node;
    NodeListEntry* next;
};

Node* parse(Token*);

#endif