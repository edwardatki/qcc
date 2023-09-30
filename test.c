char main () {
    char* terminal;
    terminal = 0x7fff;

    char test;
    test = 0;
    while (test < 10) {
        test = test + 1;
        *terminal = 0x21; // '!'
    }

    *terminal = 0x0a; // '\n'

    return 0;
}