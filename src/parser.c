#include "parser.h"
#include "print_formatting.h"

static Token* curToken;

static Node* block();
static Node* statement();

static int peek(enum TokenKind kind) {
    return curToken->kind == kind;
}

static int peek2(enum TokenKind kind) {
    return curToken->next->kind == kind;
}

static void eat() {
    // printf("%s\n", curToken->value);
    if (curToken->kind == TK_END) {
        printf("Unexpected end of tokens\n");
        exit(EXIT_FAILURE);
    }
    curToken = curToken->next;
}


static void eatKind(enum TokenKind kind) {
    char* messages[] = {"EOF", "'('", "')'", "'{'", "'}'", "','" "','", "'+'", "'-'", "'*'", "'/'", "'='", "a literal", "keyword 'return'", "an identifier", "a type", "';'", "keyword 'if'", "keyword 'else'", "'>'", "'<'", "'>='", "'<='", "'=='", "'!='", "keyword 'while'", "'&'"};
    // printf("%s %s %s\n", messages[curToken->kind], (curToken->kind == kind) ? "==" : "!=", messages[kind]);
    if (curToken->kind != kind) {
        printf("%d:%d %serror:%s expected %s but got %s\n", curToken->line, curToken->column, RED, RESET, messages[kind], messages[curToken->kind]);
        exit(EXIT_FAILURE);
    }
    eat();
}

static Type* getSymbolType(Token* token) {
    for (int i = 0; i < sizeof(baseTypes)/sizeof(baseTypes[0]); i++) {
        if (strcmp(baseTypes[i]->name, token->value) == 0) return baseTypes[i];
    }

    // Pretty sure this error can never be reached as it wouldn't get through the lexer
    printf("%d:%d %serror:%s unknown type '%s'\n", curToken->line, curToken->column, RED, RESET, token->value);
    exit(EXIT_FAILURE);
}

static void addNodeListEntry(NodeListEntry** rootEntry, Node* entryNode) {
    if (*rootEntry == NULL) {
        *rootEntry = calloc(1, sizeof(NodeListEntry));
    }

    NodeListEntry* curEntry = *rootEntry;

    *rootEntry = curEntry;
 
    // Travel to end of list
    while (curEntry->next != NULL) {
        curEntry = curEntry->next;
    }

    // Create new entry
    NodeListEntry* newEntry = calloc(1, sizeof(NodeListEntry));
    newEntry->node = entryNode;
    newEntry->next = NULL;
    curEntry->next = newEntry;
}

static Node* newNode(Token* token, enum NodeKind kind) {
    Node* node = calloc(1, sizeof(Node));
    node->token = token;
    node->kind = kind;
    node->scope = getCurrentScope();
    return node;
}

static Node* expr();

// assignment : variable ASSIGN expr
static Node* assignment () {
    // Lookup symbol from current scope
    Node* variableNode = newNode(curToken, N_VARIABLE);
    variableNode->Variable.symbol = lookupSymbol(curToken);

    Token* token = curToken;
    eat(TK_ID);
    eat(TK_ASSIGN);
    Node* node = newNode(token, N_ASSIGNMENT);
    node->Assignment.variable = variableNode;
    node->Assignment.expr = expr();

    return node;
}

// factor : ((PLUS | MINUS) factor) | ((ASTERISK | AMPERSAND) variable) | NUMBER | ID | assignment | LPAREN expr RPAREN
static Node* factor () {
    if (peek(TK_PLUS) || peek(TK_MINUS)) {
        Node* node = newNode(curToken, N_UNARY);
        eat();
        node->UnaryOp.left = factor();
        return node;
    } else if (peek(TK_ASTERISK) || peek(TK_AMPERSAND)) {
        // TODO this won't allow something like *(pointer + 1)
        // Should use an expr (maybe starting at additive_expr actually) instead of a just variable

        int mustBePointer = 0;
        if (peek(TK_ASTERISK)) mustBePointer = 1;

        Node* node = newNode(curToken, N_UNARY);
        eat();

        // Just to give a clearer error message
        if (!peek(TK_ID)) eatKind(TK_ID);

        // Lookup symbol from current scope
        Node* variableNode = newNode(curToken, N_VARIABLE);
        variableNode->Variable.symbol = lookupSymbol(curToken);

        // Must be a pointer to be dereferenced
        if (mustBePointer) {
            if (variableNode->Variable.symbol->type->kind != TY_POINTER) {
                printf("%d:%d %serror:%s left must be a pointer\n", variableNode->token->line, variableNode->token->column, RED, RESET);
                exit(EXIT_FAILURE);
            }
        }

        eat(TK_ID);

        node->UnaryOp.left = variableNode;
        return node;
    } else if (peek(TK_NUMBER)) {
        Node* node = newNode(curToken, N_NUMBER);
        eat();
        return node;
    } else if (peek(TK_ID)) {
        if (peek2(TK_ASSIGN)) {
            return assignment();
        } else {
            // Lookup symbol from current scope
            Node* node = newNode(curToken, N_VARIABLE);
            node->Variable.symbol = lookupSymbol(curToken);
            eat(TK_ID);
            return node;
        }
    } else if (peek(TK_LPAREN)) {
        eat();
        Node* node = expr();
        eat(TK_RPAREN);
        return node;
    }

    return NULL;
}

// term : factor ((MUL | DIV) factor)*
Node* term () {
    Node* factorNode = factor();
    Node* node = factorNode;

    while (peek(TK_ASTERISK) || peek(TK_DIV)) {
        Token* token = curToken;
        eat();
        node = newNode(token, N_BINOP);
        node->BinOp.left = factorNode;
        node->BinOp.right = term();
    }
    
    return node;
}

// additive_expr : term ((PLUS | MINUS) term)*
Node* additive_expr () {
    Node* node = term();

    while (peek(TK_PLUS) || peek(TK_MINUS)) {
        Token* token = curToken;
        eat();
        Node* oldNode = node;
        node = newNode(token, N_BINOP);
        node->BinOp.left = oldNode;
        node->BinOp.right = term();
    }

    return node;
}

// relational_expr : additive_expr ((MORE | LESS | MORE_EQUAL | LESS_EQUAL) additive_expr)*
Node* relational_expr () {
    Node* node = additive_expr();

    while (peek(TK_MORE) || peek(TK_LESS) || peek(TK_MORE_EQUAL) || peek(TK_LESS_EQUAL)) {
        Token* token = curToken;
        eat();
        Node* oldNode = node;
        node = newNode(token, N_BINOP);
        node->BinOp.left = oldNode;
        node->BinOp.right = additive_expr();
    }

    return node;
}

// equality_expr : relational_expr ((EQUAL | NOT_EQUAL) relational_expr)*
Node* equality_expr () {
    Node* node = relational_expr();

    while (peek(TK_EQUAL) || peek(TK_NOT_EQUAL)) {
        Token* token = curToken;
        eat();
        Node* oldNode = node;
        node = newNode(token, N_BINOP);
        node->BinOp.left = oldNode;
        node->BinOp.right = relational_expr();
    }

    return node;
}

// expr : equality_expr
static Node* expr() {
    return equality_expr();
}

// return_statement : RETURN expr
static Node* return_statement() {
    Token* token = curToken;
    eatKind(TK_RETURN);
    Node* node = newNode(token, N_RETURN);
    node->Return.expr = expr();
    return node;
}

// return_statement : IF LPAREN expr RPAREN statement (ELSE statement)?
static Node* if_statement() {
    Token* token = curToken;
    eatKind(TK_IF);

    Node* node = newNode(token, N_IF);

    eatKind(TK_LPAREN);
    node->If.expr = expr();
    eatKind(TK_RPAREN);

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
static Node* while_statement() {
    Token* token = curToken;
    eatKind(TK_WHILE);

    Node* node = newNode(token, N_WHILE);

    eatKind(TK_LPAREN);
    node->While.expr = expr();
    eatKind(TK_RPAREN);

    node->While.loop_statement = statement();

    return node;
}

// statement : (return_statement SEMICOLON) | (if_statement) | (block) | (expr SEMICOLON)
static Node* statement() {
    Node* node;

    if (peek(TK_RETURN)) {
        node = return_statement();
        eatKind(TK_SEMICOLON);
    } else if (peek(TK_IF)) {
        node = if_statement();
    } else if (peek(TK_WHILE)) {
        node = while_statement();
    } else if (peek(TK_LBRACE)) {
        node = block();
    } else {
        node = expr();
        eatKind(TK_SEMICOLON);
    }

    return node;
}

// type : TYPE (ASTERISK)
static Type* type() {
    Type* type = getSymbolType(curToken);
    eatKind(TK_TYPE);

    while (peek(TK_ASTERISK)) {
        eat();
        type = pointerTo(type);
    }

    return type;
}

// var_decl : type ID SEMICOLON
static Node* var_decl() {
    Type* symbolType = type();
    Node* node = newNode(curToken, N_VAR_DECL);

    Symbol* symbol = calloc(1, sizeof(Symbol));
    symbol->type = symbolType;

    symbol->token = curToken;

    scopeAddSymbol(symbol);

    node->VarDecl.symbol = symbol;
    eatKind(TK_ID);
    return node;
}

// Should maybe just treat var_decl as another type of statement
// block : LBRACE (statement | var_decl SEMICOLON)* RBRACE
static Node* block() {
    enterNewScope();

    Node* node = newNode(curToken, N_BLOCK);
    eatKind(TK_LBRACE);
    while (!peek(TK_RBRACE)) {
        if (peek(TK_TYPE)) {
            addNodeListEntry(&node->Block.variableDeclarations, var_decl());
            eatKind(TK_SEMICOLON);
        }
        else {
            addNodeListEntry(&node->Block.statements, statement());
        }
    }
    eatKind(TK_RBRACE);

    exitScope();

    return node;
}

// Probably should be a list of statements rather than a block, would make code generation a bit neater
// function_decl : type ID LPAREN (FORMAL_PARAMETERS)? RPAREN block
static Node* function_decl() {
    enterNewScope();

    Type* symbolType = type();
    Node* node = newNode(curToken, N_FUNC_DECL);
    eatKind(TK_ID);
    eatKind(TK_LPAREN);

    if (!peek(TK_RPAREN)) {
        addNodeListEntry(&node->FunctionDecl.formalParameters, var_decl());
        while (peek(TK_COMMA)) {
            eatKind(TK_COMMA);
            addNodeListEntry(&node->FunctionDecl.formalParameters, var_decl());
        }
    }

    eatKind(TK_RPAREN);

    node->FunctionDecl.returnType = symbolType;
    node->FunctionDecl.block = block();

    exitScope();

    return node;
}

Node* parse(Token* firstToken) {
    curToken = firstToken;

    // Global scope
    enterNewScope();

    Node* rootNode = function_decl();
    
    exitScope();

    return rootNode;
}