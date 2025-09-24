#include <stddef.h>
#include "tokens.h"

keyword kwtab[] = {
    {"begin", BEGIN},
    {"end", END},
    {"integer", INTEGER},
    {"if", IF},
    {"then", THEN},
    {"else", ELSE},
    {"function", FUNCTION},
    {"read", READ},
    {"write", WRITE},
    {"==", EQU},
    {"<>", NEQ},
    {"<=", LE},
    {"<", LT},
    {">=", GE},
    {">", GT},
    {"-", MINUS},
    {"*", MUL},
    {":=", ASSIGN},
    {"(", OPENPAREN},
    {")", CLOSEPAREN},
    {";", SEMICOLON},
};

const size_t kwtab_size = sizeof(kwtab) / sizeof(kwtab[0]);