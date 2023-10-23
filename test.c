char* terminal = 0x7fff;

char param_test(char a, int b, char* c) {
    return 0;
}

char asdf() {
    char i = 7;
    *terminal = '!';
    return i;
}

char main() {
    char test = 0;
    while (test < 10) {
        char c = *terminal;
        if (c != 0) {
            test = test + 1;
            *terminal = c;
        }
    }

    char x = asdf();

    *terminal = '\n';

    return x;
}