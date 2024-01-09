#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "messages.h"
#include "parser.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"
#include "list.h"

static struct Token* current_token;

static struct Node* block();
static struct Node* statement();

static int peek(enum TokenKind kind) {
    return current_token->kind == kind;
}

static int peek2(enum TokenKind kind) {
    return current_token->next->kind == kind;
}

static int peek3(enum TokenKind kind) {
    return current_token->next->next->kind == kind;
}

static void eat() {
    if (current_token->kind == TK_END) {
        error(current_token, "unexpected end of tokens");
    }
    current_token = current_token->next;
}


static void eat_kind(enum TokenKind kind) {
    char* messages[] = {"EOF", "'('", "')'", "'{'", "'}'", "','", "'+'", "'-'", "'*'", "'/'", "'='", "a literal", "keyword 'return'", "an identifier", "a type", "';'", "keyword 'if'", "keyword 'else'", "'>'", "'<'", "'>='", "'<='", "'=='", "'!='", "keyword 'while'", "'&'", "'|'", "'<<'", "'>>'", "a string literal", "'++'", "'--'", "keyword 'extern'"};
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

static struct Node* new_node(struct Token* token, enum NodeKind kind) {
    struct Node* node = calloc(1, sizeof(struct Node));
    node->token = token;
    node->kind = kind;
    node->scope = get_current_scope();
    node->constant = 0;
    return node;
}

static struct Node* expr();
static struct Node* additive_expr();

static struct Node* function_call() {
    // Lookup symbol from current scope
    struct Node* node = new_node(current_token, N_FUNC_CALL);
    node->FuncCall.symbol = lookup_symbol(current_token);
    node->type = node->FuncCall.symbol->type->base;
    eat_kind(TK_ID);

    eat_kind(TK_LPAREN);

    if (!peek(TK_RPAREN)) {
        struct List* current_entry = node->FuncCall.symbol->type->parameters;
        if (current_entry == NULL) error(current_token, "too many arguments to function");
        do {
            struct Type* formal_param = (struct Type*)current_entry->value;

            struct Node* expr_node = expr();

            // TODO move a bunch of this to type.c
            if ((expr_node->type->kind == TY_POINTER) && (formal_param->kind == TY_INT)) {
                warning(expr_node->token, "passing argument makes pointer from integer without a cast" );
                expr_node->type = formal_param;
            } else if ((formal_param->kind == TY_INT) && (expr_node->type->kind == TY_POINTER)) {
                warning(expr_node->token, "passing argument makes integer from pointer without a cast");
                expr_node->type = formal_param;
            } else {
                expr_node->type = get_common_type(expr_node->token, formal_param, expr_node->type);
            }

            if (expr_node->type != formal_param) error(expr_node->token, "expected parameter of type '%s' but got '%s'", formal_param->name, expr_node->type->name);
            
            list_add(&node->FuncCall.parameters, expr_node);
            
            // If not at the end of expected parameters then should see a comma
            if (current_entry->next != NULL) {
                if (!peek(TK_COMMA)) error(current_token, "expected parameter of type '%s'", ((struct Type*)current_entry->next->value)->name);
                eat();
            }
        } while (list_next(&current_entry));
    } else {
        if (node->FuncCall.symbol->type->parameters != NULL) error(node->token, "no parameters provided");
    }

    eat_kind(TK_RPAREN);

    return node;
}

// factor : (PLUS | MINUS) factor | (AMPERSAND) (INC | DEC) variable (INC | DEC) | ASTERISK factor | NUMBER | STRING | ID | ID LPAREN (expr COMMA)* RPAREN | assignment | LPAREN expr RPAREN
static struct Node* factor () {
    if (peek(TK_PLUS) || peek(TK_MINUS)) {
        struct Node* node = new_node(current_token, N_UNARY);
        eat();
        node->UnaryOp.left = factor();
        node->type = node->UnaryOp.left->type;
        return node;
    } else if (peek(TK_AMPERSAND)) {
        struct Node* node = new_node(current_token, N_UNARY);
        eat();

        // Just to give a clearer error message
        if (!peek(TK_ID)) eat_kind(TK_ID);

        // Lookup symbol from current scope
        struct Node* left_node = new_node(current_token, N_VARIABLE);
        left_node->Variable.symbol = lookup_symbol(current_token);
        left_node->type = left_node->Variable.symbol->type;
        eat_kind(TK_ID);

        node->UnaryOp.left = left_node;

        node->type = pointer_to(node->UnaryOp.left->type);

        return node;
    } else if (peek(TK_ASTERISK)) {
        struct Node* node = new_node(current_token, N_UNARY);
        eat();

        struct Node* left_node = factor();

        if (left_node->type->kind != TY_POINTER) {
            error(left_node->token, "left must be a pointer");
        }

        node->UnaryOp.left = left_node;

        node->type = node->UnaryOp.left->type->base;

        return node;
    } else if (peek(TK_NUMBER)) {
        struct Node* node = new_node(current_token, N_NUMBER);
        eat();

        if (node->token->value[0] == '\'') {
            // TODO check only a single character, escape sequences and all
            node->token->value[0] = '\"';
            int i = 1;
            while (node->token->value[i] != '\'') i++;
            node->token->value[i] = '\"';
            node->type = &type_char;
        } else {
            char* end;
            long value = strtol(node->token->value, &end, 0);
            if (end == node->token->value) error(node->token, "unable to parse number");
            if (*end != '\0') error(node->token, "unable to parse number");

            if (value > 255) node->type = &type_int;
            else node->type = &type_char;
        }

        node->constant = 1;
        return node;
    } else if (peek(TK_STRING)) {
        struct Node* node = new_node(current_token, N_STRING);
        eat();

        node->type = pointer_to(&type_char);

        node->constant = 1;
        return node;
    } else if (peek(TK_ID) || peek(TK_INC) || peek(TK_DEC)) {
        if (peek2(TK_LPAREN) && !(peek(TK_INC) || peek(TK_DEC))) {
            return function_call();
        }
        else {
            struct Token *preop_token = NULL;
            if (peek(TK_INC) || peek(TK_DEC)) {
                preop_token = current_token;
                eat();
            }

            // Lookup symbol from current scope
            struct Node* node = new_node(current_token, N_VARIABLE);
            node->Variable.symbol = lookup_symbol(current_token);
            node->type = node->Variable.symbol->type;
            eat();

            // TODO these are kind of hacked on, should really be handled more in the generator
            if (preop_token != NULL) {
                struct Node* assignment_node = new_node(preop_token, N_ASSIGNMENT);
                struct Node* unary_node = new_node(preop_token, N_UNARY);

                // Inc or dec
                unary_node->UnaryOp.left = node;
                unary_node->type = node->type;

                // Assign back to variable
                assignment_node->Assignment.left = node;
                assignment_node->Assignment.right = unary_node;
                assignment_node->type = node->type;

                node = assignment_node;
            } else { // Only allow pre or post not both, pre has priority
                if (peek(TK_INC) || peek(TK_DEC)) {
                    struct Node* assignment_node = new_node(current_token, N_ASSIGNMENT);
                    struct Node* unary_node = new_node(current_token, N_UNARY);
                    struct Node* second_unary_node = new_node(current_token, N_UNARY);

                    // Inc or dec
                    unary_node->UnaryOp.left = node;
                    unary_node->type = node->type;

                    // Assign back to variable
                    assignment_node->Assignment.left = node;
                    assignment_node->Assignment.right = unary_node;
                    assignment_node->type = node->type;

                    // Undo operation lol, this is a stupid implementation
                    second_unary_node->UnaryOp.left = assignment_node;
                    second_unary_node->type = node->type;
                    second_unary_node->token = duplicate_token(current_token);
                    if (second_unary_node->token->kind == TK_INC) second_unary_node->token->kind = TK_DEC;
                    else second_unary_node->token->kind = TK_INC;

                    node = second_unary_node;

                    eat();
                }
            }

            return node;
        }
    } else if (peek(TK_LPAREN)) {
        eat();
        struct Node* node = expr();
        eat_kind(TK_RPAREN);
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
        node->constant = node->BinOp.left->constant && node->BinOp.right->constant;
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
        node->constant = node->BinOp.left->constant && node->BinOp.right->constant;
    }

    return node;
}

// shift_expr : additive_expr ((LSHIFT | RSHIFT) additive_expr)*
static struct Node* shift_expr () {
    struct Node* node = additive_expr();

    while (peek(TK_LSHIFT) || peek(TK_RSHIFT)) {
        struct Token* token = current_token;
        eat();
        struct Node* old_node = node;
        node = new_node(token, N_BINOP);
        node->BinOp.left = old_node;
        node->BinOp.right = additive_expr();
        node->type = get_common_type(token, node->BinOp.left->type, node->BinOp.right->type);
        node->constant = node->BinOp.left->constant && node->BinOp.right->constant;
    }

    return node;
}

// relational_expr : shift_expr ((MORE | LESS | MORE_EQUAL | LESS_EQUAL) shift_expr)*
static struct Node* relational_expr () {
    struct Node* node = shift_expr();

    while (peek(TK_MORE) || peek(TK_LESS) || peek(TK_MORE_EQUAL) || peek(TK_LESS_EQUAL)) {
        struct Token* token = current_token;
        eat();
        struct Node* old_node = node;
        node = new_node(token, N_BINOP);
        node->BinOp.left = old_node;
        node->BinOp.right = shift_expr();
        node->type = get_common_type(token, node->BinOp.left->type, node->BinOp.right->type);
        node->constant = node->BinOp.left->constant && node->BinOp.right->constant;
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
        node->constant = node->BinOp.left->constant && node->BinOp.right->constant;
    }

    return node;
}

// logical_expr : equality_expr ((AND | OR) equality_expr)*
static struct Node* logical_expr () {
    struct Node* node = equality_expr();

    while (peek(TK_AMPERSAND) || peek(TK_BAR)) {
        Token* token = current_token;
        eat();
        struct Node* old_node = node;
        node = new_node(token, N_BINOP);
        node->BinOp.left = old_node;
        node->BinOp.right = equality_expr();
        node->type = get_common_type(token, node->BinOp.left->type, node->BinOp.right->type);
        node->constant = node->BinOp.left->constant && node->BinOp.right->constant;
    }

    return node;
}

// assignment : logical_expr (ASSIGN assignment)
static struct Node* assignment () {
    struct Node* node = logical_expr();
    
    if (peek(TK_ASSIGN)) {
        struct Token* token = current_token;
        eat();
        struct Node* expr = node;
        node = new_node(token, N_ASSIGNMENT);
        node->Assignment.left = expr;
        node->Assignment.right = assignment();
        node->type = expr->type;
        node->constant = node->Assignment.right->constant;

        // TODO move a bunch of this to type.c
        if ((node->type->kind == TY_POINTER) && (node->Assignment.right->type->kind == TY_INT)) {
            warning(node->token, "assignment to '%s' from '%s' makes pointer from integer without a cast", node->type->name, node->Assignment.right->type->name);
        } else if ((node->type->kind == TY_INT) && (node->Assignment.right->type->kind == TY_POINTER)) {
            warning(node->token, "assignment to '%s' from '%s' makes integer from pointer without a cast", node->type->name, node->Assignment.right->type->name);
        } else if (get_common_type(node->token, node->type, node->Assignment.right->type)->kind != node->type->kind) {
            // error(node->token, "cannot assign '%s' to '%s'", node->Assignment.right->type->name, node->type->name);
            warning(node->token, "assignment makes '%s' from '%s' without a cast", node->type->name, node->Assignment.right->type->name);
        }
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

// var_decl : (EXTERN) type ID (ASSIGN assignment)
static struct Node* var_decl() {
    int is_extern = 0;
    if (peek(TK_EXTERN)) {
        eat();
        is_extern = 1;
    }

    struct Type* symbolType = type();
    struct Node* node = new_node(current_token, N_VAR_DECL);

    struct Symbol* symbol = calloc(1, sizeof(struct Symbol));
    symbol->type = symbolType;
    symbol->token = current_token;
    symbol->is_extern = is_extern;

    scope_add_symbol(symbol);

    node->VarDecl.symbol = symbol;
    node->type = symbol->type;
    
    if (peek2(TK_ASSIGN)) {
        node->VarDecl.assignment = assignment();

        if (symbol->global) {
            if (!node->VarDecl.assignment->constant) error(node->VarDecl.assignment->token, "initializer element is not constant");
        }
    } else {
        node->VarDecl.assignment = NULL;
        eat_kind(TK_ID);
    }

    return node;
}

// block : LBRACE (statement | var_decl SEMICOLON)* RBRACE
static struct Node* block() {
    enter_new_scope();

    struct Node* node = new_node(current_token, N_BLOCK);
    node->type = &type_void;
    eat_kind(TK_LBRACE);
    while (!peek(TK_RBRACE)) {
        if (peek(TK_EXTERN) || peek(TK_TYPE)) {
            list_add(&node->Block.statements, var_decl());
            eat_kind(TK_SEMICOLON);
        } else {
            list_add(&node->Block.statements, statement());
        }
    }
    eat_kind(TK_RBRACE);

    exit_scope();

    return node;
}

// Probably should be a list of statements rather than a block, would make code generation a bit neater
// function_decl : type ID LPAREN (FORMAL_PARAMETERS)? RPAREN block
static struct Node* function_decl() {
    struct Type* symbolType = function_of(type());
    struct Node* node = new_node(current_token, N_FUNC_DECL);
    node->type = symbolType;

    struct Symbol* symbol = calloc(1, sizeof(struct Symbol));
    symbol->type = symbolType;
    symbol->token = current_token;

    scope_add_symbol(symbol);

    eat_kind(TK_ID);
    eat_kind(TK_LPAREN);

    enter_new_scope();

    if (!peek(TK_RPAREN)) {
        while (1) {
            struct Node* var_decl_node = var_decl();
            list_add(&node->FunctionDecl.formal_parameters, var_decl_node);
            add_parameter(node->type, var_decl_node->type);
            
            if (!peek(TK_COMMA)) break;
            eat();
        }
    }

    eat_kind(TK_RPAREN);

    get_current_scope()->stack_size += 2; // Return address

    node->FunctionDecl.block = block();

    exit_scope();

    return node;
}

// program : (function_decl | var_decl)*
static struct Node* program () {
    struct Node* node = new_node(current_token, N_PROGRAM);
    node->type = &type_void;
    while (!peek(TK_END)) {
        struct Token* revert_token = current_token;
        while (!peek(TK_END)) {
            if (peek(TK_ASSIGN) || peek(TK_SEMICOLON)) {
                current_token = revert_token;
                list_add(&node->Program.global_variables, var_decl());
                eat_kind(TK_SEMICOLON);
                break;
            } else if (peek(TK_LPAREN)) {
                current_token = revert_token;
                list_add(&node->Program.function_declarations, function_decl());
                break;
            } else {
                eat();
            }
        }
    }
    return node;
}

struct Node* parse(Token* first_token) {
    current_token = first_token;

    // Global scope
    enter_new_scope();

    struct Node* root_node = program();
    
    exit_scope();

    return root_node;
}