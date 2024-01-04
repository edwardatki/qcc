// Test pass by pointer
void test(char* p) {
    *p = *p - 5;
}

char main() {
    char x = 5;

    test(&x);

    return x;
}