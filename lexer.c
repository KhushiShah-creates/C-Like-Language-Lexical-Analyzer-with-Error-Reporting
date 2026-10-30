/*
 * C-Like Language Lexical Analyzer with Error Reporting
 *
 * Compile:  gcc lexer.c -o lexer
 * Run:      ./lexer test.c
 *
 * The program reads a source file one character at a time and groups the
 * characters into tokens. It reports lexical errors with line and column,
 * builds a symbol table of identifiers and prints a token summary.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_SRC   100000
#define MAX_LEN   256
#define MAX_ITEMS 1000

/* token categories */
enum { KEYWORD, IDENTIFIER, NUMBER, STRING, CHARACTER,
       OPERATOR, DELIMITER, PREPROCESSOR, TYPES };

const char *type_names[TYPES] = {
    "KEYWORD", "IDENTIFIER", "NUMBER", "STRING",
    "CHARACTER", "OPERATOR", "DELIMITER", "PREPROCESSOR"
};

const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while", NULL
};

/* operators made of two characters */
const char *operators2[] = {
    "==", "!=", "<=", ">=", "&&", "||", "++", "--", "+=", "-=",
    "*=", "/=", "%=", "<<", ">>", "->", NULL
};

struct Error {
    int line, col;
    char msg[48];
    char lexeme[64];
};

struct Symbol {
    char name[MAX_LEN];
    int first_line;
    int count;
};

struct Error  errors[MAX_ITEMS];
struct Symbol symbols[MAX_ITEMS];
int error_count = 0, symbol_count = 0;
int counts[TYPES];
int comment_count = 0;

char src[MAX_SRC];
int pos = 0, line = 1, col = 1;

/* look at a character without consuming it */
int peek(int k) {
    return (unsigned char)src[pos + k];
}

/* consume one character and keep line and column up to date */
int next(void) {
    int c = peek(0);
    pos++;
    if (c == '\n') { line++; col = 1; }
    else           { col++; }
    return c;
}

/* consume one character and store it in buf */
void take(char *buf, int *n) {
    int c = next();
    if (*n < MAX_LEN - 1) buf[(*n)++] = (char)c;
}

int is_keyword(const char *s) {
    for (int i = 0; keywords[i]; i++)
        if (strcmp(s, keywords[i]) == 0) return 1;
    return 0;
}

int is_operator2(int a, int b) {
    for (int i = 0; operators2[i]; i++)
        if (operators2[i][0] == a && operators2[i][1] == b) return 1;
    return 0;
}

/* accepts 123, 3.14 and hex such as 0x1F. Rejects 12abc and 1.2.3 */
int is_valid_number(const char *s) {
    int dots = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        if (s[2] == '\0') return 0;
        for (int i = 2; s[i]; i++)
            if (!isxdigit((unsigned char)s[i])) return 0;
        return 1;
    }
    for (int i = 0; s[i]; i++) {
        if (s[i] == '.') dots++;
        else if (!isdigit((unsigned char)s[i])) return 0;
    }
    return dots <= 1;
}

void add_token(int type, int l, int c, const char *lexeme) {
    counts[type]++;
    printf("%-6d %-6d %-14s %s\n", l, c, type_names[type], lexeme);
}

void add_error(int l, int c, const char *msg, const char *lexeme) {
    if (error_count >= MAX_ITEMS) return;
    errors[error_count].line = l;
    errors[error_count].col = c;
    snprintf(errors[error_count].msg, sizeof(errors[error_count].msg), "%s", msg);
    snprintf(errors[error_count].lexeme, sizeof(errors[error_count].lexeme), "%s", lexeme);
    error_count++;
}

void add_symbol(const char *name, int l) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].name, name) == 0) {
            symbols[i].count++;
            return;
        }
    }
    if (symbol_count < MAX_ITEMS) {
        snprintf(symbols[symbol_count].name, MAX_LEN, "%s", name);
        symbols[symbol_count].first_line = l;
        symbols[symbol_count].count = 1;
        symbol_count++;
    }
}

/* reads a string "..." or a character constant '...' */
void read_quoted(int quote, int l, int c) {
    char buf[MAX_LEN];
    int n = 0, closed = 0;

    take(buf, &n);                          /* opening quote */
    while (peek(0) != '\0' && peek(0) != '\n') {
        int ch = peek(0);
        take(buf, &n);
        if (ch == '\\' && peek(0) != '\0' && peek(0) != '\n') {
            take(buf, &n);                  /* escaped character such as \n or \" */
        } else if (ch == quote) {
            closed = 1;
            break;
        }
    }
    buf[n] = '\0';

    if (!closed) {
        add_error(l, c, quote == '"' ? "Unterminated string"
                                     : "Unterminated character constant", buf);
    } else if (quote == '"') {
        add_token(STRING, l, c, buf);
    } else if (n == 2) {
        add_error(l, c, "Empty character constant", buf);
    } else if ((n == 3 && buf[1] != '\\') || (n == 4 && buf[1] == '\\')) {
        add_token(CHARACTER, l, c, buf);
    } else {
        add_error(l, c, "Invalid character constant", buf);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source file>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Error: cannot open file %s\n", argv[1]);
        return 1;
    }
    size_t len = fread(src, 1, MAX_SRC - 1, fp);
    fclose(fp);

    /* remove carriage returns so Windows line endings do not matter */
    int j = 0;
    for (size_t i = 0; i < len; i++)
        if (src[i] != '\r') src[j++] = src[i];
    src[j] = '\0';

    printf("TOKENS\n");
    printf("%-6s %-6s %-14s %s\n", "LINE", "COL", "TYPE", "LEXEME");

    int line_start = 1;     /* 1 while we are still at the start of a line */

    while (peek(0) != '\0') {
        int c = peek(0);
        int l = line, cl = col;     /* where this token starts */
        char buf[MAX_LEN];
        int n = 0;

        if (isspace(c)) {
            next();
        }
        else if (c == '/' && peek(1) == '/') {              /* single line comment */
            while (peek(0) != '\0' && peek(0) != '\n') next();
            comment_count++;
        }
        else if (c == '/' && peek(1) == '*') {              /* block comment */
            next(); next();
            while (peek(0) != '\0' && !(peek(0) == '*' && peek(1) == '/')) next();
            if (peek(0) == '\0') {
                add_error(l, cl, "Unterminated comment", "/*");
            } else {
                next(); next();
                comment_count++;
            }
        }
        else if (c == '#' && line_start) {                  /* #include, #define */
            while (peek(0) != '\0' && peek(0) != '\n') take(buf, &n);
            buf[n] = '\0';
            add_token(PREPROCESSOR, l, cl, buf);
        }
        else if (isalpha(c) || c == '_') {                  /* keyword or identifier */
            while (isalnum(peek(0)) || peek(0) == '_') take(buf, &n);
            buf[n] = '\0';
            if (is_keyword(buf)) {
                add_token(KEYWORD, l, cl, buf);
            } else {
                add_token(IDENTIFIER, l, cl, buf);
                add_symbol(buf, l);
            }
        }
        else if (isdigit(c)) {                              /* number */
            while (isalnum(peek(0)) || peek(0) == '.' || peek(0) == '_') take(buf, &n);
            buf[n] = '\0';
            if (is_valid_number(buf)) add_token(NUMBER, l, cl, buf);
            else                      add_error(l, cl, "Malformed number", buf);
        }
        else if (c == '"' || c == '\'') {                   /* string or character */
            read_quoted(c, l, cl);
        }
        else if (is_operator2(c, peek(1))) {                /* two character operator */
            take(buf, &n); take(buf, &n);
            buf[n] = '\0';
            add_token(OPERATOR, l, cl, buf);
        }
        else if (strchr("+-*/%=<>!&|^~?:", c)) {            /* one character operator */
            take(buf, &n);
            buf[n] = '\0';
            add_token(OPERATOR, l, cl, buf);
        }
        else if (strchr("(){}[];,.", c)) {                  /* delimiter */
            take(buf, &n);
            buf[n] = '\0';
            add_token(DELIMITER, l, cl, buf);
        }
        else {                                              /* anything else is an error */
            take(buf, &n);
            buf[n] = '\0';
            add_error(l, cl, "Invalid character", buf);
        }

        if (c == '\n')          line_start = 1;
        else if (!isspace(c))   line_start = 0;
    }

    printf("\nLEXICAL ERRORS\n");
    if (error_count == 0) {
        printf("No lexical errors found.\n");
    } else {
        printf("%-6s %-6s %-32s %s\n", "LINE", "COL", "ERROR", "LEXEME");
        for (int i = 0; i < error_count; i++)
            printf("%-6d %-6d %-32s %s\n", errors[i].line, errors[i].col,
                   errors[i].msg, errors[i].lexeme);
    }

    printf("\nSYMBOL TABLE\n");
    printf("%-6s %-24s %-12s %s\n", "NO", "NAME", "FIRST LINE", "COUNT");
    for (int i = 0; i < symbol_count; i++)
        printf("%-6d %-24s %-12d %d\n", i + 1, symbols[i].name,
               symbols[i].first_line, symbols[i].count);

    printf("\nTOKEN SUMMARY\n");
    int total = 0;
    for (int i = 0; i < TYPES; i++) {
        printf("%-14s %d\n", type_names[i], counts[i]);
        total += counts[i];
    }
    printf("%-14s %d\n", "TOTAL TOKENS", total);
    printf("%-14s %d\n", "COMMENTS", comment_count);
    printf("%-14s %d\n", "ERRORS", error_count);

    return 0;
}