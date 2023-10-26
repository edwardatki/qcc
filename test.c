void putc(char c) {
    char* terminal = 0x7fff;
    *terminal = c;
}

char getc() {
    char* terminal = 0x7fff;
    return *terminal;
}

char asdf(char a) {
    char b = 7;
    putc('!');
    return a+b;
}

char main() {
    char test = 0;
    while (test < 10) {
        char c = getc();
        if (c != 0) {
            test = test + 1;
            putc(c);
        }
    }

    char x = asdf(2);

    putc('\n');

    return x;
}