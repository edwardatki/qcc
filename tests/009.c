// Test bit shift
char main() {
    char x = 0x08;
    x = x >> 3;
    x = x << 2;
    return x & 0xfb;
}