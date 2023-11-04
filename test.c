char* str = "asdf";

char getc() {
    char* terminal = 0x7000;
    return *terminal;
}

void putc(char c) {
    char* terminal = 0x7000;
    *terminal = c;
}

void print(char* str) {
    char c = *str;
    while (c != '\0') {
        putc(*str);
        str++;
        c = *str;
    }
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
    print(str);
    putc('\n');
    print("Hello world!");
    putc('\n');

    return 0;
}