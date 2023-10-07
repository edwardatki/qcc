char asdf() {
    char i;
    i = 7;
    return i;
}

// void put_char(char c) {
//     char* terminal;
//     terminal = 0x7fff;
//     *terminal = c;
// }

char main() {
    char* terminal;
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

    *terminal = 0x0a; // '\n'

    return asdf();
}