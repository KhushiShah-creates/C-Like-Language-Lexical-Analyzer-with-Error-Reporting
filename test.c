#include <stdio.h>
#define MAX 100

/* program to test the lexer */
int main() {
    int count = 10;        // valid declaration
    float price = 3.14;
    int total = 0x1F;
    char grade = 'A';
    char *msg = "hello world";

    if (count >= 5 && price != 0) {
        total += count * 2;
        count++;
    }

    int 9abc = 5;
    float bad = 1.2.3;
    char *s = "no closing quote;
    char c = 'ab';
    total = total + 5 @ 2;
    count = count # 3;
    return 0;
}
/* this comment never ends