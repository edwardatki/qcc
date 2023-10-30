void putc(char c) {
    char* terminal = 0x7fff;
    *terminal = c;
}

char getc() {
    char* terminal = 0x7fff;
    return *terminal;
}

void print_hex_u8(char value) {
    char high = (value >> 4) & 0x0f;
    if (high > 0x9) high = high - 0xA + 'A';
    else high = high + '0';
    putc(high);

    char low = value & 0x0f;
    if (low > 0x9) low = low - 0xA + 'A';
    else low = low + '0';
    putc(low);
}

char main() {
    char test = 0;
    while (test < 10) {
        char c = getc();
        if (c != 0) {
            test = test + 1;
            print_hex_u8(c);
            putc(' ');
        }
    }

    putc('\n');

    return 0;
}