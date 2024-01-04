// Test variable scopes
char main() {
    char return_code = 0;

    char x = 8;
    if (x != 8) return_code = 1;
    {
        char x = 6;
        if (x != 6) return_code = 1;
        x = 5;
        if (x != 5) return_code = 1;
    }
    if (x != 8) return_code = 1;
    x = 7;
    if (x != 7) return_code = 1;

    
    return return_code;
}