// Test pointer referencing and dereferencing
char main() {
    char x;
    char *p;

    x = 5;
    p = &x;
    *p = 0;
    
    return *p;
}