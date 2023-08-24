#ifndef _LEXER_H
#define _LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

enum TokenType {T_END=0, T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_COMMA, T_PLUS, T_MINUS, T_MUL, T_DIV, T_ASSIGN, T_NUMBER, T_RETURN, T_ID, T_TYPE, T_SEMICOLON};

typedef struct Token Token;
struct Token {
    enum TokenType type;
    char *value;
    Token *next;
};

Token* lex(FILE*);

#endif