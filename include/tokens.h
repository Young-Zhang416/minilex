#ifndef TOKENS_H
#define TOKENS_H

#include <stddef.h>


enum {
    BEGIN = 1,
    END,
    INTEGER,
    IF,
    THEN,
    ELSE,
    FUNCTION,
    READ,
    WRITE,
    IDENT,
    CONST,
    EQU,
    NEQ,
    LE,
    LT,
    GE,
    GT,
    MINUS,
    MUL,
    ASSIGN,
    OPENPAREN,
    CLOSEPAREN,
    SEMICOLON,
};

typedef struct {
    char* name;
    int code;
} keyword;

extern keyword kwtab[];
extern const size_t kwtab_size;

#endif