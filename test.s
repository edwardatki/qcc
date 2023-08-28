char main () {
    char* result;
    char test;

    result = &test;
    test = *result;

    return 0;
}