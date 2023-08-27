char main () {
    char* result;
    char test;

    result = &test;
    test = -(*result)+1;

    return 0;
}