#include "lexer.h"

static Token* curToken;

static Token* newToken(enum TokenType type) {
    Token* token = calloc(1, sizeof(Token));
    token->type = type;
    return token;
}

static void addToken(enum TokenType type, char* value) {
    Token* token = newToken(T_END);
    curToken->next = token;
    curToken->type = type;
    curToken->value = value;
    curToken = token;
}

static char next(FILE* fp) {
    char c = getc(fp);
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
    char* value = calloc(32, sizeof(char));
    int i = 0;
    while(1) {
        value[i++] = c;
        if (!isdigit(peek(fp))) break;
        c = tolower(fgetc(fp));
    }

    addToken(T_NUMBER, value);
}

static void checkKeyword(FILE* fp, char c) {
    char* value = calloc(32, sizeof(char));
    int i = 0;
    while(1) {
        value[i++] = c;
        if (!isalnum(peek(fp))) break;
        c = tolower(fgetc(fp));
    }

    if (strcmp(value, "return") == 0) addToken(T_RETURN, value);
    else if (strcmp(value, "uint8") == 0) addToken(T_TYPE, value);
    else addToken(T_ID, value);
}

Token* lex(FILE* fp) {
    Token* firstToken = newToken(T_END);
    curToken = firstToken;

    while (1) {
        char c = next(fp);
        
        if (c == EOF) {addToken(T_END, NULL); break;}
        if (isWhitespace(c)) continue;

        if (c == '(') {addToken(T_LPAREN, "{"); continue;}
        if (c == ')') {addToken(T_RPAREN, "}"); continue;}
        if (c == '{') {addToken(T_LBRACE, "{"); continue;}
        if (c == '}') {addToken(T_RBRACE, "}"); continue;}
        if (c == ',') {addToken(T_COMMA, ","); continue;}
        if (c == '+') {addToken(T_PLUS, "+"); continue;}
        if (c == '-') {addToken(T_MINUS, "-"); continue;}
        if (c == '*') {addToken(T_MUL, "*"); continue;}
        if (c == '/') {addToken(T_DIV, "/"); continue;}
        if (c == '=') {addToken(T_ASSIGN, "="); continue;}
        if (c == ';') {addToken(T_SEMICOLON, ";"); continue;}

        if (isdigit(c)) {checkNumeric(fp, c); continue;}
        if (isalpha(c)) {checkKeyword(fp, c); continue;}
    }

    return firstToken;
}