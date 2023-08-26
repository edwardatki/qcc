char test1 = 123;
char test2 = 0xea;
char test3 = 0b01010101;
char test4 = 'a';
char* pointer1 = &test1;
char test5 = *pointer1;

char divide (char value, char divisor) {
    char i = 0;
    while (value > 0) {
        value = value - divisor;
        i = i + 1;
    }
    return i;
}

char main () {
    divide(7, 2);
    return 0;
}