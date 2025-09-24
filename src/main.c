#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#define SYMTAB_INIT_CAP 128
#define CONSTAB_INIT_CAP 128

FILE* src, * dyd, *err;
int line = 1;
char ch;
char* token = NULL;

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
} ;

typedef struct {
    char* name;
    int code;
} keyword;

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

typedef struct {
    char* name;
} sbl;

sbl* symtab = NULL;
int symtab_size = 0;
int symtab_capacity = 0;

int* constab = NULL;
int constab_size = 0;
int constab_capacity = 0;

void get_char(){
    ch = fgetc(src);
    return;
}

void get_nbc(){
    while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'){
        if(ch == '\n') {
            fprintf(dyd, "EOLN 24\n");
            line++;
        }
        get_char();
    }
    return;
}

bool is_alpha(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_digit(char c){
    return c >= '0' && c <= '9';
}

bool is_alnum(char c){
    return is_alpha(c) || is_digit(c);
}

void retract(){
    if (ch != EOF){
        fseek(src, -1, SEEK_CUR);
    }
    ch = 0;
    return;
}

void concat(){
    // concat token with ch
    size_t len = token ? strlen(token) : 0;
    token = realloc(token, len + 2);
    if(token) {
        token[len] = ch;
        token[len + 1] = '\0';
    }
    return;
}

int reserve(){
    for(int i = 0; i < sizeof(kwtab) / sizeof(keyword); i++){
        if(strcmp(token, kwtab[i].name) == 0){
            fprintf(dyd, "%s %d\n", kwtab[i].name, kwtab[i].code);
            free(token);
            token = NULL;
            return kwtab[i].code;
        }
    }
    return 0;
}

int symbol(){
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
        symtab = realloc(symtab, sizeof(sbl) * symtab_capacity);
        if(!symtab){
            fprintf(err, "LINE:%d Memory allocation failed for symbol table\n", line);
            exit(EXIT_FAILURE);
        }
    }
    symtab[symtab_size].name = strdup(token);
    fprintf(dyd, "%s %d\n", token, IDENT);
    symtab_size++;
    free(token);
    token = NULL;
    return symtab_size - 1;
}

int constant(){
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
        constab = realloc(constab, sizeof(int) * constab_capacity);
        if (!constab) {
            fprintf(err, "LINE:%d Memory allocation failed for constant table\n", line);
            exit(EXIT_FAILURE);
        }
    }
    constab[constab_size] = value;
    fprintf(dyd, "%d %d\n", value, CONST);
    constab_size++;
    free(token);
    token = NULL;
    return constab_size - 1;
}

void run() {
    get_char();
    while (ch != EOF) {
        get_nbc();
        
        if (ch == EOF) break;
        
        if (is_alpha(ch)) {
            token = malloc(1);
            token[0] = '\0';
            
            while (is_alnum(ch)) {
                concat();
                get_char();
            }
            retract(); 
            
            if (!reserve()) {
                symbol();
            }
        }
        else if (is_digit(ch)) {
            token = malloc(1);
            token[0] = '\0';
            
            while (is_digit(ch)) {
                concat();
                get_char();
            }

            // detect if there are alphabetic characters after digits
            // for example: 123abc is invalid
            if(is_alpha(ch)){
                while(is_alnum(ch)){
                    concat();
                    get_char();
                }
                retract(); // retract the last non-alnum character
                fprintf(err, "LINE:%d Invalid identifier: %s\n", line, token);
                free(token);
                token = NULL;
            }else{
                retract();
                constant();
            }
        }
        else {
            token = malloc(2);
            token[0] = ch;
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
            for (int i = 0; i < sizeof(kwtab) / sizeof(keyword); i++) {
                if (strcmp(token, kwtab[i].name) == 0) {
                    fprintf(dyd, "%s %d\n", kwtab[i].name, kwtab[i].code);
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                fprintf(err, "LINE:%d Invalid character: %s\n", line, token);
            }
            
            free(token);
            token = NULL;
        }
        
        get_char();
    }
    
    fprintf(dyd, "EOF 25\n");
}

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: ./minilex <source_file>\n");
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    src = fopen(filename, "r");
    if(src == NULL){
        fprintf(stderr, "Error opening source file: %s\n", filename);
        return EXIT_FAILURE;
    }

    char dyd_filename[256], err_filename[256];
    snprintf(dyd_filename, sizeof(dyd_filename), "%s.dyd", filename);
    snprintf(err_filename, sizeof(err_filename), "%s.err", filename);

    dyd = fopen(dyd_filename, "w");
    err = fopen(err_filename, "w");
    if(dyd == NULL || err == NULL){
        fprintf(stderr, "Error creating output files: %s\n", strerror(errno));
        if(dyd) fclose(dyd);
        if(err) fclose(err);
        fclose(src);
        return EXIT_FAILURE;
    }

    run();

    fclose(dyd);
    fclose(err);
    fclose(src);

    for(int i = 0; i < symtab_size; i++){
        free(symtab[i].name);
    }
    free(symtab);
    free(constab);

    return EXIT_SUCCESS;
}