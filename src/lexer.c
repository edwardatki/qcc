#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "messages.h"
#include "symbol.h"
#include "type.h"

char* current_filename;
Token* current_token;

int current_line;
int current_column;

struct Token* new_token(enum TokenKind kind) {
    struct Token* token = calloc(1, sizeof(Token));
    token->kind = kind;
    return token;
}

struct Token* duplicate_token(struct Token* token) {
    struct Token* new_token = calloc(1, sizeof(Token));
    memcpy(new_token, token, sizeof(Token));
    return new_token;
}

static void add_token(enum TokenKind kind, char* value, int line, int column) {
    struct Token* token = new_token(TK_END);
    current_token->next = token;
    current_token->kind = kind;
    current_token->value = value;
    current_token->filename = current_filename;
    if (value != NULL) current_token->length = strlen(value);
    else current_token->length = 0;
    current_token->line = line;
    current_token->column = column;
    current_token = token;
}

static char next(FILE* fp) {
    char c = getc(fp);
    current_column++;
    return c;
}

static char peek(FILE* fp) {
    char c = getc(fp);
    ungetc(c, fp);
    return c;
}

static int is_whitespace(char c) {
    if (c == ' ') return 1;
    if (c == '\t') return 1;
    return 0;
}

char* get_line(struct Token* token) {
    FILE *fp = fopen(token->filename, "r");
    char* line = calloc(100, sizeof(char));

    int i = 1;
    while (fgets(line, 100, fp) != NULL) {
        if (i == token->line) {
            fseek(fp, 0, SEEK_SET);
            return line;
        }
        i++;
    }

    fclose(fp);
    return line;
}

static void check_preprocessor(FILE* fp, char c) {
    int start_column = current_column;

    char* value = calloc(32, sizeof(char));
    int i = 0;
    while(1) {
        value[i++] = c;
        if (!(isalnum(peek(fp)) || (peek(fp) == '_'))) break;
        c = next(fp);
    }

    if (strcmp(value, "#include") == 0) {
        char* new_filename = calloc(32, sizeof(char));
        int i = 0;
        while(1) { // Skip whitespace
            c = next(fp);
            if (c == '\"') break;
        }
        while(1) { // Get filename
            c = next(fp);
            if (c == '\"') break;
            new_filename[i++] = c;
        }

        // TODO error message from included files are broken
        char* tmp_filename = current_filename;
        char tmp_line = current_line;
        char tmp_column = current_column;

        // printf("include %s start\n", new_filename);
        lex(new_filename);
        // printf("include %s end\n", new_filename);
        
        current_filename = tmp_filename;
        current_line = tmp_line;
        current_column = tmp_column;
    } else {
        printf("%s:%d:%d: ", current_filename, current_line, current_column); 
        error(NULL, "invalid preprocessing directive %s", value);
    }

    free(value);
}

static void check_char_literal(FILE* fp, char c) {
    int start_column = current_column;

    char* value = calloc(32, sizeof(char));
    int i = 0;
    value[i++] = c;
    while(1) {
        c = next(fp);
        value[i++] = c;
        if (c == '\'') break;
    }

    add_token(TK_NUMBER, value, current_line, start_column);
}

static void check_string_literal(FILE* fp, char c) {
    int start_column = current_column;

    char* value = calloc(128, sizeof(char));
    int i = 0;
    value[i++] = c;
    while(1) {
        c = next(fp);
        value[i++] = c;
        if (c == '\"') break;
    }

    add_token(TK_STRING, value, current_line, start_column);
}

static void check_numeric(FILE* fp, char c) {
    int start_column = current_column;

    char* value = calloc(32, sizeof(char));
    int i = 0;
    while(1) {
        value[i++] = c;
        // if (!isdigit(peek(fp))) break;
        if (!isalnum(peek(fp))) break;
        c = next(fp);
    }

    add_token(TK_NUMBER, value, current_line, start_column);
}

static void check_keyword(FILE* fp, char c) {
    int start_column = current_column;

    char* value = calloc(32, sizeof(char));
    int i = 0;
    while(1) {
        value[i++] = c;
        if (!(isalnum(peek(fp)) || (peek(fp) == '_'))) break;
        c = next(fp);
    }

    // Check if a defined type
    for (int i = 0; i < sizeof(base_types)/sizeof(base_types[0]); i++) {
        if (strcmp(value, base_types[i]->name) == 0) {
            add_token(TK_TYPE, value, current_line, start_column); 
            return;
        }
    }

    if (strcmp(value, "return") == 0) add_token(TK_RETURN, value, current_line, start_column);
    else if (strcmp(value, "if") == 0) add_token(TK_IF, value, current_line, start_column);
    else if (strcmp(value, "else") == 0) add_token(TK_ELSE, value, current_line, start_column);
    else if (strcmp(value, "while") == 0) add_token(TK_WHILE, value, current_line, start_column);
    else if (strcmp(value, "extern") == 0) add_token(TK_EXTERN, value, current_line, start_column);
    else if (strcmp(value, "NULL") == 0) add_token(TK_NUMBER, "0", current_line, start_column); // Just kinda bodged a NULL in here lol
    else add_token(TK_ID, value, current_line, start_column);
}

// TODO Things could be easier if we just read the whole file into memory
// Token values could just point into the file data and we add a length variable
struct Token* lex(char* _filename) {
    current_filename = _filename;
    current_line = 1;
    current_column = 0;

    FILE *fp = fopen(current_filename, "r");
    if (!fp) error(NULL, "unable to open file '%s'", current_filename);

    static struct Token* first_token;
    if (first_token == NULL) {
        first_token = new_token(TK_END);
        current_token = first_token;
    }

    while (1) {
        char c = next(fp);

        // Track line number
        if (c =='\n') {
            current_line++;
            current_column = 0;
            continue;
        }

        if (c == '\r') continue;

        // Exit at end of file
        if (c == EOF) {
            break;
        }

        // Skip whitespace
        if (is_whitespace(c)) {
            continue;
        }

        // Skip comments
        if ((c == '/') && (peek(fp) == '/')) {
            while ((peek(fp) != '\n') && (peek(fp) != EOF)) next(fp);
            continue;
        }

        // Check double character tokens
        if ((c == '>') && (peek(fp) == '=')) {add_token(TK_MORE_EQUAL, ">=", current_line, current_column); next(fp); continue;}
        if ((c == '<') && (peek(fp) == '=')) {add_token(TK_LESS_EQUAL, "<=", current_line, current_column); next(fp); continue;}
        if ((c == '=') && (peek(fp) == '=')) {add_token(TK_EQUAL, "==", current_line, current_column); next(fp); continue;}
        if ((c == '!') && (peek(fp) == '=')) {add_token(TK_NOT_EQUAL, "!=", current_line, current_column); next(fp); continue;}
        if ((c == '<') && (peek(fp) == '<')) {add_token(TK_LSHIFT, "<<", current_line, current_column); next(fp); continue;}
        if ((c == '>') && (peek(fp) == '>')) {add_token(TK_RSHIFT, ">>", current_line, current_column); next(fp); continue;}
        if ((c == '+') && (peek(fp) == '+')) {add_token(TK_INC, "++", current_line, current_column); next(fp); continue;}
        if ((c == '-') && (peek(fp) == '-')) {add_token(TK_DEC, "--", current_line, current_column); next(fp); continue;}

        // Check single character tokens
        if (c == '(') {add_token(TK_LPAREN, "{", current_line, current_column); continue;}
        if (c == ')') {add_token(TK_RPAREN, "}", current_line, current_column); continue;}
        if (c == '{') {add_token(TK_LBRACE, "{", current_line, current_column); continue;}
        if (c == '}') {add_token(TK_RBRACE, "}", current_line, current_column); continue;}
        if (c == ',') {add_token(TK_COMMA, ",", current_line, current_column); continue;}
        if (c == '+') {add_token(TK_PLUS, "+", current_line, current_column); continue;}
        if (c == '-') {add_token(TK_MINUS, "-", current_line, current_column); continue;}
        if (c == '*') {add_token(TK_ASTERISK, "*", current_line, current_column); continue;}
        if (c == '/') {add_token(TK_DIV, "/", current_line, current_column); continue;}
        if (c == '=') {add_token(TK_ASSIGN, "=", current_line, current_column); continue;}
        if (c == ';') {add_token(TK_SEMICOLON, ";", current_line, current_column); continue;}
        if (c == '>') {add_token(TK_MORE, ">", current_line, current_column); continue;}
        if (c == '<') {add_token(TK_LESS, "<", current_line, current_column); continue;}
        if (c == '&') {add_token(TK_AMPERSAND, "&", current_line, current_column); continue;}
        if (c == '|') {add_token(TK_BAR, "|", current_line, current_column); continue;}

        // Check multi character tokens
        if (isdigit(c)) {check_numeric(fp, c); continue;}
        if (isalpha(c)) {check_keyword(fp, c); continue;}
        if (c == '\'') {check_char_literal(fp, c); continue;}
        if (c == '\"') {check_string_literal(fp, c); continue;}

        if (c == '#') {check_preprocessor(fp, c); continue;}

        printf("%s:%d:%d: ", current_filename, current_line, current_column); 
        error(NULL, "unrecognized token");
    }

    fclose(fp);

    return first_token;
}