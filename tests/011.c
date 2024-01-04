// Test a recursive function call
char test(char depth) {
    if (depth == 0) return 50;
    else return test(depth - 1) - 1;
}

char main() {
    return test(50);
}