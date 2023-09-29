#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "messages.h"
#include "parser.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"

static struct Token* current_token;

static struct Node* block();
static struct Node* statement();

static int peek(enum TokenKind kind) {
    return current_token->kind == kind;
}

static int peek2(enum TokenKind kind) {
    return current_token->next->kind == kind;
}

static void eat() {
    if (current_token->kind == TK_END) {
        error(current_token, "unexpected end of tokens");
    }
    current_token = current_token->next;
}


static void eat_kind(enum TokenKind kind) {
    char* messages[] = {"EOF", "'('", "')'", "'{'", "'}'", "','" "','", "'+'", "'-'", "'*'", "'/'", "'='", "a literal", "keyword 'return'", "an identifier", "a type", "';'", "keyword 'if'", "keyword 'else'", "'>'", "'<'", "'>='", "'<='", "'=='", "'!='", "keyword 'while'", "'&'"};
    if (current_token->kind != kind) {
        error(current_token, "expected %s but got %s", messages[kind], messages[current_token->kind]);
    }
    eat();
}

static struct Type* get_symbol_type(struct Token* token) {
    for (int i = 0; i < sizeof(base_types)/sizeof(base_types[0]); i++) {
        if (strcmp(base_types[i]->name, token->value) == 0) return base_types[i];
    }

    // Pretty sure this error can never be reached as it wouldn't get through the lexer
    error(current_token, "unknown type '%s'", token->value);
}

static void add_node_list_entry(struct NodeListEntry** root_entry, struct Node* entryNode) {
    if (*root_entry == NULL) {
        *root_entry = calloc(1, sizeof(struct NodeListEntry));
    }

    struct NodeListEntry* current_entry = *root_entry;

    *root_entry = current_entry;
 
    // Travel to end of list
    while (current_entry->next != NULL) {
        current_entry = current_entry->next;
    }

    // Create new entry
    struct NodeListEntry* new_entry = calloc(1, sizeof(struct NodeListEntry));
    new_entry->node = entryNode;
    new_entry->next = NULL;
    current_entry->next = new_entry;
}

static struct Node* new_node(struct Token* token, enum NodeKind kind) {
    struct Node* node = calloc(1, sizeof(struct Node));
    node->token = token;
    node->kind = kind;
    node->scope = get_current_scope();
    return node;
}

static struct Node* expr();

// factor : ((PLUS | MINUS) factor) | ((ASTERISK | AMPERSAND) variable) | NUMBER | ID | assignment | LPAREN expr RPAREN
static struct Node* factor () {
    if (peek(TK_PLUS) || peek(TK_MINUS)) {
        struct Node* node = new_node(current_token, N_UNARY);
        eat();
        node->UnaryOp.left = factor();
        node->type = node->UnaryOp.left->type;
        return node;
    } else if (peek(TK_ASTERISK) || peek(TK_AMPERSAND)) {
        // TODO this won't allow something like *(pointer + 1)
        // Should use an expr (maybe starting at additive_expr actually) instead of a just variable

        int must_be_pointer = 0;
        if (peek(TK_ASTERISK)) must_be_pointer = 1;

        struct Node* node = new_node(current_token, N_UNARY);
        eat();

        // Just to give a clearer error message
        if (!peek(TK_ID)) eat_kind(TK_ID);

        // Lookup symbol from current scope
        struct Node* variable_node = new_node(current_token, N_VARIABLE);
        variable_node->Variable.symbol = lookup_symbol(current_token);
        variable_node->type = variable_node->Variable.symbol->type;

        // Must be a pointer to be dereferenced
        if (must_be_pointer) {
            if (variable_node->Variable.symbol->type->kind != TY_POINTER) {
                error(variable_node->token, "left must be a pointer");
            }
        }

        eat(TK_ID);

        node->UnaryOp.left = variable_node;

        if (must_be_pointer) node->type = node->UnaryOp.left->type->base;
        else node->type = pointer_to(node->UnaryOp.left->type);

        return node;
    } else if (peek(TK_NUMBER)) {
        struct Node* node = new_node(current_token, N_NUMBER);

        int value = atoi(node->token->value);
        if (value > 255) node->type = &type_int;
        else node->type = &type_char;

        eat();
        return node;
    } else if (peek(TK_ID)) {
        // Lookup symbol from current scope
        struct Node* node = new_node(current_token, N_VARIABLE);
        node->Variable.symbol = lookup_symbol(current_token);
        node->type = node->Variable.symbol->type;
        eat(TK_ID);
        return node;
    } else if (peek(TK_LPAREN)) {
        eat();
        struct Node* node = expr();
        eat(TK_RPAREN);
        return node;
    }

    return NULL;
}

// term : factor ((MUL | DIV) factor)*
static struct Node* term () {
    struct Node* factor_node = factor();
    struct Node* node = factor_node;

    while (peek(TK_ASTERISK) || peek(TK_DIV)) {
        struct Token* token = current_token;
        eat();
        node = new_node(token, N_BINOP);
        node->BinOp.left = factor_node;
        node->BinOp.right = term();
        node->type = get_common_type(token, node->BinOp.left->type, node->BinOp.right->type);
    }
    
    return node;
}

// additive_expr : term ((PLUS | MINUS) term)*
static struct Node* additive_expr () {
    struct Node* node = term();

    while (peek(TK_PLUS) || peek(TK_MINUS)) {
        struct Token* token = current_token;
        eat();
        struct Node* old_node = node;
        node = new_node(token, N_BINOP);
        node->BinOp.left = old_node;
        node->BinOp.right = term();
        node->type = get_common_type(token, node->BinOp.left->type, node->BinOp.right->type);
    }

    return node;
}

// relational_expr : additive_expr ((MORE | LESS | MORE_EQUAL | LESS_EQUAL) additive_expr)*
static struct Node* relational_expr () {
    struct Node* node = additive_expr();

    while (peek(TK_MORE) || peek(TK_LESS) || peek(TK_MORE_EQUAL) || peek(TK_LESS_EQUAL)) {
        struct Token* token = current_token;
        eat();
        struct Node* old_node = node;
        node = new_node(token, N_BINOP);
        node->BinOp.left = old_node;
        node->BinOp.right = additive_expr();
        node->type = get_common_type(token, node->BinOp.left->type, node->BinOp.right->type);
    }

    return node;
}

// equality_expr : relational_expr ((EQUAL | NOT_EQUAL) relational_expr)*
static struct Node* equality_expr () {
    struct Node* node = relational_expr();

    while (peek(TK_EQUAL) || peek(TK_NOT_EQUAL)) {
        Token* token = current_token;
        eat();
        struct Node* old_node = node;
        node = new_node(token, N_BINOP);
        node->BinOp.left = old_node;
        node->BinOp.right = relational_expr();
        node->type = get_common_type(token, node->BinOp.left->type, node->BinOp.right->type);
    }

    return node;
}

// assignment : equality_expr (ASSIGN assignment)
static struct Node* assignment () {
    struct Node* node = equality_expr();
    
    if (peek(TK_ASSIGN)) {
        struct Token* token = current_token;
        eat();
        struct Node* expr = node;
        node = new_node(token, N_ASSIGNMENT);
        node->Assignment.left = expr;
        node->Assignment.right = assignment();

        node->type = get_common_type(token, node->Assignment.left->type, node->Assignment.right->type);
    }

    return node;
}

// expr : assignment
static struct Node* expr() {
    return assignment();
}

// return_statement : RETURN expr
static struct Node* return_statement() {
    Token* token = current_token;
    eat_kind(TK_RETURN);
    struct Node* node = new_node(token, N_RETURN);
    node->Return.expr = expr();
    node->type = node->Return.expr->type;
    return node;
}

// return_statement : IF LPAREN expr RPAREN statement (ELSE statement)?
static struct Node* if_statement() {
    struct Token* token = current_token;
    eat_kind(TK_IF);

    struct Node* node = new_node(token, N_IF);

    eat_kind(TK_LPAREN);
    node->If.expr = expr();
    node->type = node->If.expr->type;
    eat_kind(TK_RPAREN);

    node->If.true_statement = statement();

    if (peek(TK_ELSE)) {
        eat();
        node->If.false_statement = statement();
    }
    else {
        node->If.false_statement = NULL;
    }

    return node;
}

// while_statement : WHILE LPAREN expr RPAREN statement
static struct Node* while_statement() {
    struct Token* token = current_token;
    eat_kind(TK_WHILE);

    struct Node* node = new_node(token, N_WHILE);

    eat_kind(TK_LPAREN);
    node->While.expr = expr();
    node->type = node->While.expr->type;
    eat_kind(TK_RPAREN);

    node->While.loop_statement = statement();

    return node;
}

// statement : (return_statement SEMICOLON) | (if_statement) | (block) | (expr SEMICOLON)
static struct Node* statement() {
    struct Node* node;

    if (peek(TK_RETURN)) {
        node = return_statement();
        eat_kind(TK_SEMICOLON);
    } else if (peek(TK_IF)) {
        node = if_statement();
    } else if (peek(TK_WHILE)) {
        node = while_statement();
    } else if (peek(TK_LBRACE)) {
        node = block();
    } else {
        node = expr();
        eat_kind(TK_SEMICOLON);
    }

    return node;
}

// type : TYPE (ASTERISK)
static struct Type* type() {
    struct Type* type = get_symbol_type(current_token);
    eat_kind(TK_TYPE);

    while (peek(TK_ASTERISK)) {
        eat();
        type = pointer_to(type);
    }

    return type;
}

// var_decl : type ID SEMICOLON
static struct Node* var_decl() {
    struct Type* symbolType = type();
    struct Node* node = new_node(current_token, N_VAR_DECL);

    struct Symbol* symbol = calloc(1, sizeof(struct Symbol));
    symbol->type = symbolType;

    symbol->token = current_token;

    scope_add_symbol(symbol);

    node->VarDecl.symbol = symbol;
    node->type = symbol->type;
    
    eat_kind(TK_ID);
    return node;
}

// Should maybe just treat var_decl as another type of statement
// block : LBRACE (statement | var_decl SEMICOLON)* RBRACE
static struct Node* block() {
    enter_new_scope();

    struct Node* node = new_node(current_token, N_BLOCK);
    node->type = &type_void;
    eat_kind(TK_LBRACE);
    while (!peek(TK_RBRACE)) {
        if (peek(TK_TYPE)) {
            add_node_list_entry(&node->Block.variable_declarations, var_decl());
            eat_kind(TK_SEMICOLON);
        }
        else {
            add_node_list_entry(&node->Block.statements, statement());
        }
    }
    eat_kind(TK_RBRACE);

    exit_scope();

    return node;
}

// Probably should be a list of statements rather than a block, would make code generation a bit neater
// function_decl : type ID LPAREN (FORMAL_PARAMETERS)? RPAREN block
static struct Node* function_decl() {
    enter_new_scope();

    struct Type* symbolType = type();
    struct Node* node = new_node(current_token, N_FUNC_DECL);
    node->type = symbolType;
    eat_kind(TK_ID);
    eat_kind(TK_LPAREN);

    if (!peek(TK_RPAREN)) {
        add_node_list_entry(&node->FunctionDecl.formal_parameters, var_decl());
        while (peek(TK_COMMA)) {
            eat_kind(TK_COMMA);
            add_node_list_entry(&node->FunctionDecl.formal_parameters, var_decl());
        }
    }

    eat_kind(TK_RPAREN);

    node->FunctionDecl.block = block();

    exit_scope();

    return node;
}

struct Node* parse(Token* first_token) {
    current_token = first_token;

    // Global scope
    enter_new_scope();

    struct Node* root_node = function_decl();
    
    exit_scope();

    return root_node;
}