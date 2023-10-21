char* terminal = 0x7fff;

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