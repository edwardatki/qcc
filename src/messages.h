#ifndef __MESSAGES_H
#define __MESSAGES_H

#define BOLD        "\x1B[1m"
#define DIM         "\x1B[2m"
#define ITALIC      "\x1B[3m"
#define UNDERLINE   "\x1B[4m"
#define BLINK       "\x1B[5m"
#define INVERSE     "\x1B[7m"
#define HIDDEN      "\x1B[8m"
#define STRIKE      "\x1B[9m"

#define RED         "\x1B[31m"
#define GRN         "\x1B[32m"
#define YEL         "\x1B[33m"
#define BLU         "\x1B[34m"
#define MAG         "\x1B[35m"
#define CYN         "\x1B[36m"
#define WHT         "\x1B[37m"

#define RESET       "\x1B[0m"

struct Token;
typedef struct Token Token;

void error(Token*, const char* format, ...);
void warning(Token*, const char* format, ...);

#endif