char main () {
    char* terminal;
    terminal = 32767; // 0x7fff

    char test;
    test = 0;
    while (test < 10) {
        test = test + 1;
        *terminal = 33; // '!'
    }

    *terminal = 10; // '\n'

    return 0;
}