char* terminal;

char asdf() {
    char i;
    i = 7;
    *terminal = 0x21;
    return i;
}

char main() {
    terminal = 0x7fff;

    char test;
    test = 0;
    while (test < 10) {
        char c;
        c = *terminal;
        if (c != 0) {
            test = test + 1;
            *terminal = c;
        }
    }

    char x;
    x = asdf();

    *terminal = 0x0a; // '\n'

    return x;
}