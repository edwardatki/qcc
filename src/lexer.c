#include "lexer.h"
#include "symbol.h"

static Token* curToken;

int currentLine = 1;
int currentColumn = 0;

static Token* newToken(enum TokenKind kind) {
    Token* token = calloc(1, sizeof(Token));
    token->kind = kind;
    return token;
}

static void addToken(enum TokenKind kind, char* value, int line, int column) {
    Token* token = newToken(TK_END);
    curToken->next = token;
    curToken->kind = kind;
    curToken->value = value;
    curToken->line = line;
    curToken->column = column;
    curToken = token;
}

static char next(FILE* fp) {
    char c = getc(fp);
    currentColumn++;
    return c;
}

static char peek(FILE* fp) {
    char c = getc(fp);
    ungetc(c, fp);
    return c;
}

static int isWhitespace(char c) {
    if (c == ' ') return 1;
    if (c == '\t') return 1;
    return 0;
}

static void checkNumeric(FILE* fp, char c) {
    int startColumn = currentColumn;

    char* value = calloc(32, sizeof(char));
    int i = 0;
    while(1) {
        value[i++] = c;
        if (!isdigit(peek(fp))) break;
        c = next(fp);
    }

    addToken(TK_NUMBER, value, currentLine, startColumn);
}

static void checkKeyword(FILE* fp, char c) {
    int startColumn = currentColumn;

    char* value = calloc(32, sizeof(char));
    int i = 0;
    while(1) {
        value[i++] = c;
        if (!isalnum(peek(fp))) break;
        c = next(fp);
    }

    // Check if a defined type
    for (int i = 0; i < sizeof(baseTypes)/sizeof(baseTypes[0]); i++) {
        if (strcmp(value, baseTypes[i]->name) == 0) {
            addToken(TK_TYPE, value, currentLine, startColumn); 
            return;
        }
    }

    if (strcmp(value, "return") == 0) addToken(TK_RETURN, value, currentLine, startColumn);
    else if (strcmp(value, "if") == 0) addToken(TK_IF, value, currentLine, startColumn);
    else if (strcmp(value, "else") == 0) addToken(TK_ELSE, value, currentLine, startColumn);
    else if (strcmp(value, "while") == 0) addToken(TK_WHILE, value, currentLine, startColumn);
    else addToken(TK_ID, value, currentLine, startColumn);
}

Token* lex(FILE* fp) {
    Token* firstToken = newToken(TK_END);
    curToken = firstToken;

    while (1) {
        char c = next(fp);
        
        // Track line number
        if (c =='\n') {
            currentLine++;
            currentColumn = 0;
        }

        // Exit at end of file
        if (c == EOF) {
            addToken(TK_END, NULL, currentLine, currentColumn);
            break;
        }

        // Skip whitespace
        if (isWhitespace(c)) {
            continue;
        }

        // Skip comments
        if ((c == '/') && (peek(fp) == '/')) {
            while ((peek(fp) != '\n') && (peek(fp) != EOF)) next(fp);
            continue;
        }

        // Check double character tokens
        if ((c == '>') && (peek(fp) == '=')) {addToken(TK_MORE_EQUAL, ">=", currentLine, currentColumn); next(fp); continue;}
        if ((c == '<') && (peek(fp) == '=')) {addToken(TK_LESS_EQUAL, "<=", currentLine, currentColumn); next(fp); continue;}
        if ((c == '=') && (peek(fp) == '=')) {addToken(TK_EQUAL, "==", currentLine, currentColumn); next(fp); continue;}
        if ((c == '!') && (peek(fp) == '=')) {addToken(TK_NOT_EQUAL, "!=", currentLine, currentColumn); next(fp); continue;}

        // Check single character tokens
        if (c == '(') {addToken(TK_LPAREN, "{", currentLine, currentColumn); continue;}
        if (c == ')') {addToken(TK_RPAREN, "}", currentLine, currentColumn); continue;}
        if (c == '{') {addToken(TK_LBRACE, "{", currentLine, currentColumn); continue;}
        if (c == '}') {addToken(TK_RBRACE, "}", currentLine, currentColumn); continue;}
        if (c == ',') {addToken(TK_COMMA, ",", currentLine, currentColumn); continue;}
        if (c == '+') {addToken(TK_PLUS, "+", currentLine, currentColumn); continue;}
        if (c == '-') {addToken(TK_MINUS, "-", currentLine, currentColumn); continue;}
        if (c == '*') {addToken(TK_ASTERISK, "*", currentLine, currentColumn); continue;}
        if (c == '/') {addToken(TK_DIV, "/", currentLine, currentColumn); continue;}
        if (c == '=') {addToken(TK_ASSIGN, "=", currentLine, currentColumn); continue;}
        if (c == ';') {addToken(TK_SEMICOLON, ";", currentLine, currentColumn); continue;}
        if (c == '>') {addToken(TK_MORE, ">", currentLine, currentColumn); continue;}
        if (c == '<') {addToken(TK_LESS, "<", currentLine, currentColumn); continue;}
        if (c == '&') {addToken(TK_AMPERSAND, "&", currentLine, currentColumn); continue;}

        // Check multi character tokens
        if (isdigit(c)) {checkNumeric(fp, c); continue;}
        if (isalpha(c)) {checkKeyword(fp, c); continue;}
    }

    return firstToken;
}