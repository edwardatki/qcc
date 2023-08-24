#include "parser.h"

static Token* curToken;

static int peek(enum TokenType type) {
    return curToken->type == type;
}

static int peek2(enum TokenType type) {
    return curToken->next->type == type;
}

static void eat() {
    // printf("%s\n", curToken->value);
    if (curToken->type == T_END) {
        printf("Unexpected end of tokens\n");
        exit(EXIT_FAILURE);
    }
    curToken = curToken->next;
}

static void eatType(enum TokenType type) {
    if (curToken->type != type) {
        printf("Expected %d but got %d: %s\n", type, curToken->type, curToken->value);
        exit(EXIT_FAILURE);
    }
    eat();
}

static Type* getSymbolType(Token* token) {
    for (int i = 0; i < sizeof(builtinTypes)/sizeof(builtinTypes[0]); i++) {
        if (strcmp(builtinTypes[i].name, token->value) == 0) return &builtinTypes[i];
    }
    printf("UNKNOWN TYPE!\n");
    return NULL;
}

static void addNodeListEntry(NodeListEntry** listRoot, Node* entryNode) {
    if (*listRoot == NULL) {
        *listRoot = calloc(1, sizeof(NodeListEntry));
    }

    NodeListEntry* curEntry = *listRoot;
 
    // Travel to end of list
    // printf("%p\n", curEntry);
    while (curEntry->next != NULL) {
        curEntry = curEntry->next;
        // printf("%p\n", curEntry);
    }

    // Create new entry
    NodeListEntry* newEntry = calloc(1, sizeof(NodeListEntry));
    newEntry->node = entryNode;
    curEntry->next = newEntry;
}

static Node* newNode(Token* token, enum TokenType type) {
    Node* node = calloc(1, sizeof(Node));
    node->token = token;
    node->type = type;
    return node;
}

static Node* expr();

// assignment : variable ASSIGN expr
static Node* assignment () {
    Node* variableNode = newNode(curToken, N_VARIABLE);
    // TODO lookup symbol
    variableNode->Variable.symbol = NULL;
    Token* token = curToken;
    eat(T_ID);
    eat(T_ASSIGN);
    Node* node = newNode(token, N_ASSIGNMENT);
    node->Assignment.variable = variableNode;
    node->Assignment.expr = expr();
    return node;
}

// factor : NUMBER | ID | assignment | empty
static Node* factor () {
    if (peek(T_NUMBER)) {
        Node* node = newNode(curToken, N_NUMBER);
        eat();
        return node;
    } else if (peek(T_ID)) {
        if (peek2(T_ASSIGN)) {
            return assignment();
        } else {
            Node* node = newNode(curToken, N_VARIABLE);
            eat(T_ID);
            return node;
        }
    }

    return NULL;
}

// term : factor ((MUL | DIV) factor)*
Node* term () {
    Node* factorNode = factor();
    Node* node = factorNode;

    while (peek(T_MUL) || peek(T_DIV)) {
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
    Node* termNode = term();
    Node* node = termNode;

    while (peek(T_PLUS) || peek(T_MINUS)) {
        Token* token = curToken;
        eat();
        node = newNode(token, N_BINOP);
        node->BinOp.left = termNode;
        node->BinOp.right = term();
    }

    return node;
}

// expr : additive_expr
static Node* expr() {
    return additive_expr();
}

// return_statement : RETURN expr
static Node* return_statement() {
    Token* token = curToken;
    eatType(T_RETURN);
    Node* node = newNode(token, N_RETURN);
    node->Return.expr = expr();
    return node;
}

static Node* block();

// statement : (return_statement SEMICOLON) | (block) | (expr SEMICOLON)
static Node* statement() {
    Node* node;

    if (peek(T_RETURN)) {
        node = return_statement();
        eatType(T_SEMICOLON);
    } else if (peek(T_LBRACE)) {
        node = block();
    } else {
        node = expr();
        eatType(T_SEMICOLON);
    }

    return node;
}

// type : TYPE
static Type* type() {
    Type* symbolType = getSymbolType(curToken);
    eatType(T_TYPE);
    return symbolType;
}

// var_decl : type ID SEMICOLON
static Node* var_decl() {
    Type* symbolType = type();
    Node* node = newNode(curToken, N_VAR_DECL);
    Symbol* symbol = calloc(1, sizeof(Symbol));
    symbol->token = curToken;
    symbol->type = symbolType;
    symbol->memoryLocation = "???";
    node->VarDecl.symbol = symbol;
    eatType(T_ID);
    return node;
}

// block : LBRACE (statement | var_decl SEMICOLON)* RBRACE
static Node* block() {
    Node* node = newNode(curToken, N_BLOCK);
    eatType(T_LBRACE);
    while (!peek(T_RBRACE)) {
        if (peek(T_TYPE)) {
            addNodeListEntry(&node->Block.variableDeclarations, var_decl());
            eatType(T_SEMICOLON);
        }
        else {
            addNodeListEntry(&node->Block.statements, statement());
        }
    }
    eatType(T_RBRACE);

    return node;
}

// function_decl : type ID LPAREN (FORMAL_PARAMETERS)? RPAREN block
static Node* function_decl() {
    Type* symbolType = type();
    Node* node = newNode(curToken, N_FUNC_DECL);
    eatType(T_ID);
    eatType(T_LPAREN);

    if (!peek(T_RPAREN)) {
        addNodeListEntry(&node->FunctionDecl.formalParameters, var_decl());
        while (peek(T_COMMA)) {
            eatType(T_COMMA);
            addNodeListEntry(&node->FunctionDecl.formalParameters, var_decl());
        }
    }

    eatType(T_RPAREN);

    node->FunctionDecl.returnType = symbolType;
    node->FunctionDecl.block = block();

    return node;
}

Node* parse(Token* firstToken) {
    curToken = firstToken;

    Node* rootNode = function_decl();

    return rootNode;
}