char main () {
    char test;
    test = 1;

    {
        char test;
        test = 2;
    }

    {
        char test;
        test = 3;
    }

    return test;
}