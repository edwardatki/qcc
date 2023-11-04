#ifndef _LEXER_H
#define _LEXER_H

// This bad lol
struct _IO_FILE;
typedef struct _IO_FILE FILE;

enum TokenKind {TK_END=0, TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE, TK_COMMA, TK_PLUS, TK_MINUS, TK_ASTERISK, TK_DIV, TK_ASSIGN, TK_NUMBER, TK_RETURN, TK_ID, TK_TYPE, TK_SEMICOLON, TK_IF, TK_ELSE, TK_MORE, TK_LESS, TK_MORE_EQUAL, TK_LESS_EQUAL, TK_EQUAL, TK_NOT_EQUAL, TK_WHILE, TK_AMPERSAND, TK_BAR, TK_LSHIFT, TK_RSHIFT, TK_STRING, TK_INC, TK_DEC};

struct Token {
    enum TokenKind kind;
    char* filename;
    char *value;
    int length;
    struct Token *next;
    int line;
    int column;
};

struct Token* lex(char*);
char* get_line(int);

#endif