#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "tokens.h"
#include "lexer.h"

// Internal state
static FILE* src = NULL;
static FILE* dyd = NULL;
static FILE* errf = NULL;

static int line_no = 1;
static int ch = 0;           // use int to keep EOF
static char* token = NULL;

typedef struct {
    char* name;
} sbl;

static sbl* symtab = NULL;
static int symtab_size = 0;
static int symtab_capacity = 0;

static int* constab = NULL;
static int constab_size = 0;
static int constab_capacity = 0;

#define SYMTAB_INIT_CAP 128
#define CONSTAB_INIT_CAP 128

// Get next character
static void get_char(void){
    ch = fgetc(src);
}

// Skip blanks and emit EOLN
static void get_nbc(void){
    while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'){
        if(ch == '\n') {
            fprintf(dyd, "EOLN 24\n");
            line_no++;
        }
        get_char();
        if (ch == EOF) break;
    }
}

static bool is_alpha(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_digit(char c){
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c){
    return is_alpha(c) || is_digit(c);
}

// Retract one character (no-op on EOF)
static void retract(void){
    if (ch != EOF){
        fseek(src, -1, SEEK_CUR);
    }
    ch = 0;
}

// Append current char to token
static void concat(void){
    size_t len = token ? strlen(token) : 0;
    char* np = realloc(token, len + 2);
    if (!np) return;
    token = np;
    token[len] = (char)ch;
    token[len + 1] = '\0';
}

// Check reserved words/operators
static int reserve(void){
    for(size_t i = 0; i < kwtab_size; i++){
        if(strcmp(token, kwtab[i].name) == 0){
            fprintf(dyd, "%s %d\n", kwtab[i].name, kwtab[i].code);
            free(token);
            token = NULL;
            return kwtab[i].code;
        }
    }
    return 0;
}

// ---- Symbol/constant tables (linear) ----
static int symbol(void){
    for(int i = 0; i < symtab_size; i++){
        if(strcmp(token, symtab[i].name) == 0){
            fprintf(dyd, "%s %d\n", symtab[i].name, IDENT);
            free(token);
            token = NULL;
            return i;
        }
    }

    if(symtab_size >= symtab_capacity){
        symtab_capacity = symtab_capacity ? symtab_capacity * 2 : SYMTAB_INIT_CAP;
        sbl* np = realloc(symtab, sizeof(sbl) * symtab_capacity);
        if(!np){
            fprintf(errf, "LINE:%d Memory allocation failed for symbol table\n", line_no);
            exit(EXIT_FAILURE);
        }
        symtab = np;
    }
    symtab[symtab_size].name = strdup(token);
    fprintf(dyd, "%s %d\n", token, IDENT);
    symtab_size++;
    free(token);
    token = NULL;
    return symtab_size - 1;
}

static int constant_tok(void){
    int value = atoi(token);
    for(int i = 0; i < constab_size; i++){
        if(constab[i] == value){
            fprintf(dyd, "%d %d\n", value, CONST);
            free(token);
            token = NULL;
            return i;
        }
    }

    if(constab_size >= constab_capacity){
        constab_capacity = constab_capacity ? constab_capacity * 2 : CONSTAB_INIT_CAP;
        int* np = realloc(constab, sizeof(int) * constab_capacity);
        if (!np) {
            fprintf(errf, "LINE:%d Memory allocation failed for constant table\n", line_no);
            exit(EXIT_FAILURE);
        }
        constab = np;
    }
    constab[constab_size] = value;
    fprintf(dyd, "%d %d\n", value, CONST);
    constab_size++;
    free(token);
    token = NULL;
    return constab_size - 1;
}

// Main lexing loop
static void run_lexer_internal(void) {
    get_char();
    while (ch != EOF) {
        get_nbc();
        if (ch == EOF) break;

        if (is_alpha((char)ch)) {
            token = malloc(1);
            if (!token) exit(EXIT_FAILURE);
            token[0] = '\0';

            while (is_alnum((char)ch)) {
                concat();
                get_char();
            }
            retract();

            if (!reserve()) {
                symbol();
            }
        }
        else if (is_digit((char)ch)) {
            token = malloc(1);
            if (!token) exit(EXIT_FAILURE);
            token[0] = '\0';

            while (is_digit((char)ch)) {
                concat();
                get_char();
            }

            // Handle invalid identifier like 123abc
            if(is_alpha((char)ch)){
                while(is_alnum((char)ch)){
                    concat();
                    get_char();
                }
                retract();
                fprintf(errf, "LINE:%d Invalid identifier: %s\n", line_no, token);
                free(token);
                token = NULL;
            }else{
                retract();
                constant_tok();
            }
        }
        else {
            token = malloc(2);
            if (!token) exit(EXIT_FAILURE);
            token[0] = (char)ch;
            token[1] = '\0';

            get_char();
            if (token[0] == ':' && ch == '=') {
                concat();
                get_char();
            } else if ((token[0] == '<' || token[0] == '>') && ch == '=') {
                concat();
                get_char();
            } else if (token[0] == '<' && ch == '>') {
                concat();
                get_char();
            } else {
                retract();
            }

            int found = 0;
            for (size_t i = 0; i < kwtab_size; i++) {
                if (strcmp(token, kwtab[i].name) == 0) {
                    fprintf(dyd, "%s %d\n", kwtab[i].name, kwtab[i].code);
                    found = 1;
                    break;
                }
            }

            if (!found) {
                fprintf(errf, "LINE:%d Invalid character: %s\n", line_no, token);
            }

            free(token);
            token = NULL;
        }

        get_char();
    }

    fprintf(dyd, "EOF 25\n");
}


int lexer_run(const char* filename) {
    char dyd_filename[256], err_filename[256];

    src = fopen(filename, "r");
    if(!src){
        fprintf(stderr, "Error opening source file: %s\n", filename);
        return 1;
    }

    snprintf(dyd_filename, sizeof(dyd_filename), "%s.dyd", filename);
    snprintf(err_filename, sizeof(err_filename), "%s.err", filename);

    dyd = fopen(dyd_filename, "w");
    errf = fopen(err_filename, "w");
    if(!dyd || !errf){
        fprintf(stderr, "Error creating output files: %s\n", strerror(errno));
        if(dyd) fclose(dyd);
        if(errf) fclose(errf);
        fclose(src);
        return 2;
    }

    line_no = 1;
    ch = 0;
    free(token); token = NULL;

    run_lexer_internal();

    fclose(dyd); dyd = NULL;
    fclose(errf); errf = NULL;
    fclose(src); src = NULL;

    for (int i = 0; i < symtab_size; i++) free(symtab[i].name);

    free(symtab); symtab = NULL; symtab_size = symtab_capacity = 0;

    free(constab); constab = NULL; constab_size = constab_capacity = 0;

    return 0;
}