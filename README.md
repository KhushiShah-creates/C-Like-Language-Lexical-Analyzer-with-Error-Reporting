# C-Like Language Lexical Analyzer with Error Reporting

A lexical analyzer (scanner) written in C for the Compiler Design course. It reads a C-like source file, splits it into tokens, and reports lexical errors with the exact line and column.

## Features

- Recognizes 8 token types: keywords, identifiers, numbers, strings, character constants, operators, delimiters and preprocessor lines
- Reports the line and column of every token
- Detects and reports lexical errors, then continues scanning
- Builds a symbol table of unique identifiers with first line and usage count
- Skips single line and block comments, and counts them
- Prints a token summary with counts per type

## Lexical errors detected

| Error | Example |
|---|---|
| Invalid character | `@`, `$`, `#` in the middle of a line |
| Malformed number | `9abc`, `1.2.3` |
| Unterminated string | `"hello` |
| Unterminated comment | `/* never closed` |
| Empty character constant | `''` |
| Invalid character constant | `'ab'` |

## Files

| File | Purpose |
|---|---|
| `lexer.c` | The lexical analyzer |
| `test.c` | Sample input containing every token type and every error type |

## Build and run

Requires gcc. On MSYS2, install it with `pacman -S gcc` if it is missing.

```
gcc lexer.c -o lexer
./lexer test.c
```

To analyze any other file, pass its name as the argument:

```
./lexer yourfile.c
```

## Sample

Input:

```c
int x = 5;
x = x + 2 @ 1;
```

Output:

```
TOKENS
LINE   COL    TYPE           LEXEME
1      1      KEYWORD        int
1      5      IDENTIFIER     x
1      7      OPERATOR       =
1      9      NUMBER         5
1      10     DELIMITER      ;
2      1      IDENTIFIER     x
2      3      OPERATOR       =
2      5      IDENTIFIER     x
2      7      OPERATOR       +
2      9      NUMBER         2
2      13     NUMBER         1
2      14     DELIMITER      ;

LEXICAL ERRORS
LINE   COL    ERROR                            LEXEME
2      11     Invalid character                @

SYMBOL TABLE
NO     NAME                     FIRST LINE   COUNT
1      x                        1            3

TOKEN SUMMARY
KEYWORD        1
IDENTIFIER     3
NUMBER         3
STRING         0
CHARACTER      0
OPERATOR       3
DELIMITER      2
PREPROCESSOR   0
TOTAL TOKENS   12
COMMENTS       0
ERRORS         1
```

## How it works

1. The input file is read into a character array (`src`). A position index and a one character lookahead (`peek`) are used to scan it.
2. In each step, the first character decides what kind of token begins: whitespace, comment, preprocessor line, word, number, quoted text, operator, delimiter, or an invalid character.
3. The characters that belong together are collected into a buffer, and the complete lexeme is then classified. Words are checked against the keyword table, and numbers are validated.
4. Valid tokens are printed with their position. Identifiers are also added to the symbol table without duplicates.
5. Anything that fits no rule is stored as an error with its position, and scanning continues.
6. After the end of the file, the error report, symbol table and token summary are printed.

Line and column tracking is done in one place, the `next()` function, so every reported position is consistent.

## Limitations

- Float suffixes (`1.5f`) and exponent notation (`1e5`) are reported as malformed numbers
- Hex escapes inside character constants (`'\x41'`) are not supported
- Input files are limited to 100000 characters
- Only lexical analysis is done. There is no parsing or syntax checking
