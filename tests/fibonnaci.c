#include "io.h"

char fibonacci(char n) {
    char value;

    if(n == 0) value = 0;
    else if(n == 1) value = 1;
    else value = fibonacci(n - 1) + fibonacci(n - 2);

    return value;
}

char main() {
    char n = 0;
    while (n <= 13) {
        print("fibonacci(");
        print_hex_u8(n);
        print(") = ");
        print_hex_u8(fibonacci(n));
        print("\n");
        n++;
    }

    if (fibonacci(13) != 233) return 1;

    return 0;
}