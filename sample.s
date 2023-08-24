uint8 test1 = 123;
uint8 test2 = 0xea;
uint8 test3 = 0b01010101;
uint8 test4 = 'a';
uint8* pointer1 = &test1;
uint8 test5 = *pointer1;

uint8 divide (uint8 value, uint8 divisor) {
    uint8 i = 0;
    while (value > 0) {
        value = value - divisor;
        i = i + 1;
    }
    return i;
}

uint8_t main () {
    divide(7, 2);
    return 0;
}