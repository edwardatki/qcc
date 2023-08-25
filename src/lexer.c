#include "lexer.h"
#include "symbol.h"

static Token* curToken;

int currentLine = 1;
int currentColumn = 0;

static Token* newToken(enum TokenType type) {
    Token* token = calloc(1, sizeof(Token));
    token->type = type;
    return token;
}

static void addToken(enum TokenType type, char* value, int line, int column) {
    Token* token = newToken(T_END);
    curToken->next = token;
    curToken->type = type;
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

    addToken(T_NUMBER, value, currentLine, startColumn);
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
    for (int i = 0; i < sizeof(builtinTypes)/sizeof(builtinTypes[0]); i++) {
        if (strcmp(value, builtinTypes[i].name) == 0) {
            addToken(T_TYPE, value, currentLine, startColumn);
            return;
        }
    }

    if (strcmp(value, "return") == 0) addToken(T_RETURN, value, currentLine, startColumn);
    else if (strcmp(value, "if") == 0) addToken(T_IF, value, currentLine, startColumn);
    else if (strcmp(value, "else") == 0) addToken(T_ELSE, value, currentLine, startColumn);
    else addToken(T_ID, value, currentLine, startColumn);
}

Token* lex(FILE* fp) {
    Token* firstToken = newToken(T_END);
    curToken = firstToken;

    while (1) {
        char c = next(fp);
        
        if (c =='\n') {
            currentLine++;
            currentColumn = 0;
        }

        if (c == EOF) {
            addToken(T_END, NULL, currentLine, currentColumn);
            break;
        }

        if (isWhitespace(c)) {
            continue;
        }

        if ((c == '/') && (peek(fp) == '/')) {
            while (peek(fp) != '\n') next(fp);
            continue;
        }

        if (c == '(') {addToken(T_LPAREN, "{", currentLine, currentColumn); continue;}
        if (c == ')') {addToken(T_RPAREN, "}", currentLine, currentColumn); continue;}
        if (c == '{') {addToken(T_LBRACE, "{", currentLine, currentColumn); continue;}
        if (c == '}') {addToken(T_RBRACE, "}", currentLine, currentColumn); continue;}
        if (c == ',') {addToken(T_COMMA, ",", currentLine, currentColumn); continue;}
        if (c == '+') {addToken(T_PLUS, "+", currentLine, currentColumn); continue;}
        if (c == '-') {addToken(T_MINUS, "-", currentLine, currentColumn); continue;}
        if (c == '*') {addToken(T_MUL, "*", currentLine, currentColumn); continue;}
        if (c == '/') {addToken(T_DIV, "/", currentLine, currentColumn); continue;}
        if (c == '=') {addToken(T_ASSIGN, "=", currentLine, currentColumn); continue;}
        if (c == ';') {addToken(T_SEMICOLON, ";", currentLine, currentColumn); continue;}

        if (isdigit(c)) {checkNumeric(fp, c); continue;}
        if (isalpha(c)) {checkKeyword(fp, c); continue;}
    }

    return firstToken;
}